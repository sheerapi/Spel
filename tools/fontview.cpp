#include "core/log.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <string_view>
#include <vector>

extern "C"
{
#include "core/entry.h"
#include "core/window.h"
#include "gfx/canvas/canvas_internal.h"
#include "gfx/gfx_canvas.h"
#include "input/input_keyboard.h"
}

namespace
{

	struct FontViewConfig
	{
		std::string font_path;
		std::string text =
			"The quick brown fox jumps over the lazy dog.\n0123456789 !@#$%^&*()";
		float text_size = 72.0f;
		float sheet_size = 42.0f;
		int cols = 16;
		float padding = 16.0f;
		float cell_pad = 6.0f;

		bool show_metrics = true;
		bool show_help = true;
	};

	FontViewConfig cfg;
	std::vector<uint8_t> font_bytes;
	spel_font loaded_font = nullptr;
	spel_font ui_font = nullptr;

	float sheet_cell_w = 0.0f;
	float sheet_cell_h = 0.0f;

	enum class ViewMode : uint8_t
	{
		Type = 0,
		Sheet = 1,
		Atlas = 2,
	};

	ViewMode mode = ViewMode::Type;
	std::string edit_text;
	std::vector<const spel_font_glyph*> sorted_glyphs;
	int sheet_first = 0;
	int selected_glyph = 0;

	struct KeyRepeatState
	{
		bool down;
		float elapsed;
		float next_fire;
	};

	KeyRepeatState key_repeat[SPEL_KEY_COUNT] = {};

	static inline spel_vec2 v2(float x, float y)
	{
		spel_vec2 v;
		v.x = x;
		v.y = y;
		return v;
	}

	static inline spel_rect recti(float x, float y, float w, float h)
	{
		return spel_rect_create((int)std::lround(x), (int)std::lround(y),
								(int)std::lround(w), (int)std::lround(h));
	}

	void usage(FILE* out)
	{
		std::fprintf(
			out,
			"spel-fontview - render an .spfn font using Spel's real text pipeline\n"
			"\n"
			"Usage:\n"
			"  spel-fontview <font.spfn> [options]\n"
			"\n"
			"Options:\n"
			"  --text <str>         Sample text (supports \\n)\n"
			"  --size <px>          Sample text size (default: %.1f)\n"
			"  --sheet-size <px>    Glyph sheet size (default: %.1f)\n"
			"  --cols <n>           Glyph sheet columns (default: %d)\n"
			"  --no-metrics         Hide metric guides\n"
			"  --help               Show this help\n"
			"\n"
			"Controls:\n"
			"  Esc      Quit\n"
			"  1/2/3    Switch mode (type/sheet/atlas)\n"
			"  +/-      Change size (type: text, sheet: glyphs)\n"
			"  [ / ]    Change sheet size\n"
			"  , / .    Change sheet columns\n"
			"  M        Toggle metric guides\n"
			"  H        Toggle this help\n"
			"  R        Reload font from disk\n",
			cfg.text_size, cfg.sheet_size, cfg.cols);
	}

	bool parse_float(const char* s, float& out)
	{
		if (!s || !*s)
			return false;
		char* end = nullptr;
		float v = std::strtof(s, &end);
		if (!end || end == s || *end != '\0')
			return false;
		out = v;
		return true;
	}

	bool parse_int(const char* s, int& out)
	{
		if (!s || !*s)
			return false;
		char* end = nullptr;
		long v = std::strtol(s, &end, 10);
		if (!end || end == s || *end != '\0')
			return false;
		if (v < 1 || v > 4096)
			return false;
		out = (int)v;
		return true;
	}

	bool read_file(const std::string& path, std::vector<uint8_t>& out)
	{
		out.clear();

		FILE* f = std::fopen(path.c_str(), "rb");
		if (!f)
			return false;

		if (std::fseek(f, 0, SEEK_END) != 0)
		{
			std::fclose(f);
			return false;
		}

		long size = std::ftell(f);
		if (size <= 0)
		{
			std::fclose(f);
			return false;
		}

		if (std::fseek(f, 0, SEEK_SET) != 0)
		{
			std::fclose(f);
			return false;
		}

		out.resize((size_t)size);
		size_t n = std::fread(out.data(), 1, out.size(), f);
		std::fclose(f);
		return n == out.size();
	}

	bool bitmap_top_origin(const spel_font font)
	{
		if (!font)
			return false;

		if (font->header.font_type != SPFN_TYPE_BITMAP)
			return false;

		float min_plane_y = 0.0f;
		bool have_plane_y = false;
		for (int i = 0; i < font->header.glyph_count; i++)
		{
			const spel_font_glyph* g = &font->glyphs[i];
			if (g->plane_h <= 0.0f)
				continue;

			if (!have_plane_y)
			{
				min_plane_y = g->plane_y;
				have_plane_y = true;
			}
			else if (g->plane_y < min_plane_y)
			{
				min_plane_y = g->plane_y;
			}

			if (min_plane_y < 0.0f)
				break;
		}

		return have_plane_y && min_plane_y >= 0.0f;
	}

	float baseline_offset_y(const spel_font font, float scale)
	{
		return bitmap_top_origin(font) ? 0.0f : (font->header.ascender * scale);
	}

	std::string utf8(uint32_t cp)
	{
		std::string s;
		if (cp <= 0x7F)
		{
			s.push_back((char)cp);
			return s;
		}
		if (cp <= 0x7FF)
		{
			s.push_back((char)(0xC0 | ((cp >> 6) & 0x1F)));
			s.push_back((char)(0x80 | (cp & 0x3F)));
			return s;
		}
		if (cp <= 0xFFFF)
		{
			s.push_back((char)(0xE0 | ((cp >> 12) & 0x0F)));
			s.push_back((char)(0x80 | ((cp >> 6) & 0x3F)));
			s.push_back((char)(0x80 | (cp & 0x3F)));
			return s;
		}
		if (cp <= 0x10FFFF)
		{
			s.push_back((char)(0xF0 | ((cp >> 18) & 0x07)));
			s.push_back((char)(0x80 | ((cp >> 12) & 0x3F)));
			s.push_back((char)(0x80 | ((cp >> 6) & 0x3F)));
			s.push_back((char)(0x80 | (cp & 0x3F)));
			return s;
		}

		s.push_back('?');
		return s;
	}

	void compute_sheet_cell(const spel_font font)
	{
		sheet_cell_h = font->header.line_height * cfg.sheet_size + cfg.cell_pad * 2.0f;

		float max_em_w = 0.0f;
		for (int i = 0; i < font->header.glyph_count; i++)
		{
			const spel_font_glyph* g = &font->glyphs[i];
			max_em_w = std::max(max_em_w, std::max(g->advance, g->plane_w));
		}

		sheet_cell_w = max_em_w * cfg.sheet_size + cfg.cell_pad * 2.0f;
		if (sheet_cell_w < sheet_cell_h * 0.75f)
			sheet_cell_w = sheet_cell_h * 0.75f;
	}

	const char* font_type_to_string(uint8_t t)
	{
		switch (t)
		{
		case SPFN_TYPE_SDF:
			return "SDF";
		case SPFN_TYPE_MSDF:
			return "MSDF";
		case SPFN_TYPE_BITMAP:
			return "BITMAP";
		case SPFN_TYPE_MTSDF:
			return "MTSDF";
		default:
			return "UNKNOWN";
		}
	}

	void rebuild_sorted_glyphs()
	{
		sorted_glyphs.clear();
		if (!loaded_font)
			return;

		sorted_glyphs.reserve(loaded_font->header.glyph_count);
		for (int i = 0; i < loaded_font->header.glyph_count; i++)
		{
			sorted_glyphs.push_back(&loaded_font->glyphs[i]);
		}
		std::sort(sorted_glyphs.begin(), sorted_glyphs.end(),
				  [](const spel_font_glyph* a, const spel_font_glyph* b)
				  { return a->codepoint < b->codepoint; });

		sheet_first = 0;
		selected_glyph = 0;
	}

	bool load_font_from_disk()
	{
		if (cfg.font_path.empty())
			return false;

		if (!read_file(cfg.font_path, font_bytes))
		{
			std::fprintf(stderr, "spel-fontview: failed to read file: %s\n",
						 cfg.font_path.c_str());
			return false;
		}

		spel_font f = spel_font_create(spel.gfx, font_bytes.data(), font_bytes.size());
		if (!f)
		{
			std::fprintf(stderr, "spel-fontview: failed to load font: %s\n",
						 cfg.font_path.c_str());
			return false;
		}

		if (loaded_font)
		{
			spel_font_destroy(loaded_font);
			loaded_font = nullptr;
		}

		loaded_font = f;
		compute_sheet_cell(loaded_font);
		rebuild_sorted_glyphs();
		return true;
	}

	std::string unescape_newlines(std::string_view s)
	{
		std::string out;
		out.reserve(s.size());
		for (size_t i = 0; i < s.size(); i++)
		{
			if (s[i] == '\\' && i + 1 < s.size() && s[i + 1] == 'n')
			{
				out.push_back('\n');
				i++;
				continue;
			}
			out.push_back(s[i]);
		}
		return out;
	}

	bool parse_args()
	{
		int argc = spel.process.argc;
		const char** argv = spel.process.argv;

		if (argc < 2)
		{
			usage(stderr);
			return false;
		}

		if (std::strcmp(argv[1], "--help") == 0 || std::strcmp(argv[1], "-h") == 0)
		{
			usage(stdout);
			return false;
		}

		cfg.font_path = argv[1];

		for (int i = 2; i < argc; i++)
		{
			const char* a = argv[i];
			if (std::strcmp(a, "--help") == 0 || std::strcmp(a, "-h") == 0)
			{
				usage(stdout);
				return false;
			}
			if (std::strcmp(a, "--no-metrics") == 0)
			{
				cfg.show_metrics = false;
				continue;
			}
			if (std::strcmp(a, "--text") == 0 && i + 1 < argc)
			{
				cfg.text = unescape_newlines(argv[++i]);
				continue;
			}
			if (std::strcmp(a, "--size") == 0 && i + 1 < argc)
			{
				float v = 0.0f;
				if (!parse_float(argv[++i], v) || v <= 1.0f || v > 2048.0f)
				{
					std::fprintf(stderr, "spel-fontview: invalid --size\n");
					return false;
				}
				cfg.text_size = v;
				continue;
			}
			if (std::strcmp(a, "--sheet-size") == 0 && i + 1 < argc)
			{
				float v = 0.0f;
				if (!parse_float(argv[++i], v) || v <= 1.0f || v > 2048.0f)
				{
					std::fprintf(stderr, "spel-fontview: invalid --sheet-size\n");
					return false;
				}
				cfg.sheet_size = v;
				continue;
			}
			if (std::strcmp(a, "--cols") == 0 && i + 1 < argc)
			{
				int v = 0;
				if (!parse_int(argv[++i], v))
				{
					std::fprintf(stderr, "spel-fontview: invalid --cols\n");
					return false;
				}
				cfg.cols = v;
				continue;
			}

			std::fprintf(stderr, "spel-fontview: unknown argument: %s\n", a);
			return false;
		}

		return true;
	}

	size_t pop_last_utf8_codepoint(std::string& s)
	{
		if (s.empty())
			return 0;

		size_t i = s.size() - 1;
		while (i > 0 && (static_cast<uint8_t>(s[i]) & 0xC0) == 0x80)
		{
			i--;
		}
		size_t removed = s.size() - i;
		s.erase(i);
		return removed;
	}

	void input_consume_text(std::string& out)
	{
		uint8_t n = 0;
		const char* t = spel_input_text(&n);
		if (!t || n == 0)
			return;

		out.append(t, t + n);

		// spel_input_text_clear() disables the internal "enabled" flag, so re-start to
		// keep consuming SDL text input events.
		spel_input_text_clear();
		spel_input_text_start();
	}

	int key_repeat_count(spel_key key, float dt, float delay_s, float interval_s)
	{
		if (key < 0 || key >= SPEL_KEY_COUNT)
			return 0;

		const bool down_now = spel_input_key(key);
		KeyRepeatState& s = key_repeat[key];

		if (!down_now)
		{
			s.down = false;
			s.elapsed = 0.0f;
			s.next_fire = 0.0f;
			return 0;
		}

		if (!s.down)
		{
			s.down = true;
			s.elapsed = 0.0f;
			s.next_fire = delay_s;
			return 1;
		}

		s.elapsed += dt;

		int count = 0;
		if (interval_s <= 0.0f)
			return 0;

		if (s.next_fire <= 0.0f)
			s.next_fire = delay_s;

		while (s.elapsed >= s.next_fire)
		{
			count++;
			s.next_fire += interval_s;
			if (count > 256)
				break;
		}

		return count;
	}

	void draw_info_panel(float x, float y)
	{
		if (!loaded_font)
			return;

		const float pad = 10.0f;
		const float w = 560.0f;
		const float h = 178.0f;

		spel_canvas_fill_mode_set(SPEL_CANVAS_FILL);
		spel_canvas_color_set(spel_color_hexa(0x000000AA));
		spel_canvas_draw_rect(recti(x, y, w, h));

		spel_canvas_font_set(ui_font);
		spel_canvas_font_size_set(16.0f);
		spel_canvas_color_set(spel_color_hexa(0xFFFFFFEE));

		const char* mode_name = "type";
		switch (mode)
		{
		case ViewMode::Type:
			mode_name = "type";
			break;
		case ViewMode::Sheet:
			mode_name = "sheet";
			break;
		case ViewMode::Atlas:
			mode_name = "atlas";
			break;
		}

		const auto& h0 = loaded_font->header;
		const bool top_origin = bitmap_top_origin(loaded_font);
		spel_canvas_print(v2(x + pad, y + pad),
						  "mode: %s  (%d/3)\n"
						  "font: %s\n"
						  "type: %s  channels: %u  atlas: %ux%u\n"
						  "glyphs: %u  kerning: %u\n"
						  "em: %.2f  asc: %.3f  desc: %.3f  line: %.3f  sdf_range: %.2f\n"
						  "bitmap origin: %s",
						  mode_name, (int)mode + 1, cfg.font_path.c_str(),
						  font_type_to_string(h0.font_type), (unsigned)h0.channels,
						  (unsigned)h0.atlas_width, (unsigned)h0.atlas_height,
						  (unsigned)h0.glyph_count, (unsigned)h0.kerning_count,
						  h0.em_size, h0.ascender, h0.descender, h0.line_height,
						  h0.sdf_range, top_origin ? "top-left" : "baseline");
	}

	void draw_help_panel(float x, float y)
	{
		const float pad = 10.0f;
		const float w = 860.0f;
		const float h = 110.0f;

		spel_canvas_fill_mode_set(SPEL_CANVAS_FILL);
		spel_canvas_color_set(spel_color_hexa(0x000000AA));
		spel_canvas_draw_rect(recti(x, y, w, h));

		spel_canvas_font_set(ui_font);
		spel_canvas_font_size_set(16.0f);
		spel_canvas_color_set(spel_color_hexa(0xFFFFFFEE));

		spel_canvas_print(
			v2(x + pad, y + pad),
			"Esc quit | 1/2/3 mode | R reload | H help | M metrics\n"
			"type: Enter newline, Backspace delete, Ctrl+L clear, +/- size\n"
			"sheet: PgUp/PgDn scroll, +/- size, [ ] sheet size, ,/. cols, wasd scroll\n"
			"atlas: wasd select glyph (shift = faster)");
	}

} // namespace

extern "C" void spel_conf()
{
	spel.window.title = "Spël Font View";
	spel.window.width = 1280;
	spel.window.height = 720;
	spel.window.resizable = false;
}

extern "C" void spel_load()
{
	spel_canvas_init();
	ui_font = spel_canvas_font_monospace();

	if (!parse_args())
	{
		spel_window_close();
		return;
	}

	if (!load_font_from_disk())
	{
		usage(stderr);
		spel_window_close();
		return;
	}

	edit_text = cfg.text;
	mode = ViewMode::Type;
	sheet_first = 0;
	selected_glyph = 0;
	spel_input_text_start();
}

extern "C" void spel_update(double)
{
	const float dt = (float)spel.time.delta_unscaled;

	if (spel_input_key_pressed(SPEL_KEY_ESCAPE))
	{
		spel_window_close();
		return;
	}

	if (spel_input_key_pressed(SPEL_KEY_R))
	{
		load_font_from_disk();
	}

	if (spel_input_key_pressed(SPEL_KEY_M))
		cfg.show_metrics = !cfg.show_metrics;
	if (spel_input_key_pressed(SPEL_KEY_H))
		cfg.show_help = !cfg.show_help;

	if (spel_input_key_pressed(SPEL_KEY_1))
		mode = ViewMode::Type;
	if (spel_input_key_pressed(SPEL_KEY_2))
		mode = ViewMode::Sheet;
	if (spel_input_key_pressed(SPEL_KEY_3))
		mode = ViewMode::Atlas;

	if (mode == ViewMode::Type)
	{
		if (!spel_input_text_active())
			spel_input_text_start();

		input_consume_text(edit_text);

		int backspace = key_repeat_count(SPEL_KEY_BACKSPACE, dt, 0.35f, 0.045f);
		for (int i = 0; i < backspace; i++)
		{
			pop_last_utf8_codepoint(edit_text);
		}

		if (spel_input_key_pressed(SPEL_KEY_RETURN))
		{
			edit_text.push_back('\n');
		}

		if (spel_input_key_ctrl() && spel_input_key_pressed(SPEL_KEY_L))
		{
			edit_text.clear();
		}

		int plus = key_repeat_count(SPEL_KEY_EQUALS, dt, 0.35f, 0.06f) +
				   key_repeat_count(SPEL_KEY_KP_PLUS, dt, 0.35f, 0.06f);
		for (int i = 0; i < plus; i++)
		{
			cfg.text_size = std::min(cfg.text_size * 1.1f, 2048.0f);
		}
		int minus = key_repeat_count(SPEL_KEY_MINUS, dt, 0.35f, 0.06f) +
					key_repeat_count(SPEL_KEY_KP_MINUS, dt, 0.35f, 0.06f);
		for (int i = 0; i < minus; i++)
		{
			cfg.text_size = std::max(cfg.text_size / 1.1f, 2.0f);
		}
	}
	else
	{
		if (spel_input_text_active())
			spel_input_text_stop();

		if (mode == ViewMode::Sheet)
		{
			int plus = key_repeat_count(SPEL_KEY_EQUALS, dt, 0.35f, 0.06f) +
					   key_repeat_count(SPEL_KEY_KP_PLUS, dt, 0.35f, 0.06f);
			for (int i = 0; i < plus; i++)
			{
				cfg.sheet_size = std::min(cfg.sheet_size * 1.1f, 2048.0f);
				if (loaded_font)
					compute_sheet_cell(loaded_font);
			}
			int minus = key_repeat_count(SPEL_KEY_MINUS, dt, 0.35f, 0.06f) +
						key_repeat_count(SPEL_KEY_KP_MINUS, dt, 0.35f, 0.06f);
			for (int i = 0; i < minus; i++)
			{
				cfg.sheet_size = std::max(cfg.sheet_size / 1.1f, 2.0f);
				if (loaded_font)
					compute_sheet_cell(loaded_font);
			}

			int sheet_up = key_repeat_count(SPEL_KEY_RIGHTBRACKET, dt, 0.35f, 0.06f);
			for (int i = 0; i < sheet_up; i++)
			{
				cfg.sheet_size = std::min(cfg.sheet_size * 1.1f, 2048.0f);
				if (loaded_font)
					compute_sheet_cell(loaded_font);
			}
			int sheet_dn = key_repeat_count(SPEL_KEY_LEFTBRACKET, dt, 0.35f, 0.06f);
			for (int i = 0; i < sheet_dn; i++)
			{
				cfg.sheet_size = std::max(cfg.sheet_size / 1.1f, 2.0f);
				if (loaded_font)
					compute_sheet_cell(loaded_font);
			}

			int col_up = key_repeat_count(SPEL_KEY_PERIOD, dt, 0.35f, 0.05f);
			for (int i = 0; i < col_up; i++)
				cfg.cols = std::min(cfg.cols + 1, 128);

			int col_dn = key_repeat_count(SPEL_KEY_COMMA, dt, 0.35f, 0.05f);
			for (int i = 0; i < col_dn; i++)
				cfg.cols = std::max(cfg.cols - 1, 1);

			int page_step = std::max(1, cfg.cols * 10);
			int pd = key_repeat_count(SPEL_KEY_PAGEDOWN, dt, 0.35f, 0.08f);
			sheet_first += pd * page_step;

			int pu = key_repeat_count(SPEL_KEY_PAGEUP, dt, 0.35f, 0.08f);
			sheet_first -= pu * page_step;

			int dn = key_repeat_count(SPEL_KEY_S, dt, 0.35f, 0.04f);
			sheet_first += dn * cfg.cols;

			int up = key_repeat_count(SPEL_KEY_W, dt, 0.35f, 0.04f);
			sheet_first -= up * cfg.cols;

			int max_first = std::max(0, (int)sorted_glyphs.size() - 1);
			sheet_first = std::clamp(sheet_first, 0, max_first);
		}

		if (mode == ViewMode::Atlas)
		{
			int step = spel_input_key_shift() ? 16 : 1;

			int r = key_repeat_count(SPEL_KEY_D, dt, 0.25f, 0.035f);
			selected_glyph += r * step;

			int l = key_repeat_count(SPEL_KEY_A, dt, 0.25f, 0.035f);
			selected_glyph -= l * step;

			int d = key_repeat_count(SPEL_KEY_W, dt, 0.25f, 0.035f);
			selected_glyph += d * step * 16;

			int u = key_repeat_count(SPEL_KEY_S, dt, 0.25f, 0.035f);
			selected_glyph -= u * step * 16;

			int max_i = std::max(0, (int)sorted_glyphs.size() - 1);
			selected_glyph = std::clamp(selected_glyph, 0, max_i);
		}
	}
}

extern "C" void spel_draw()
{
	spel_canvas_begin(nullptr);
	spel_canvas_clear(spel_color_hexa(0x0E1117FF));

	if (!loaded_font)
	{
		spel_canvas_end();
		return;
	}

	const float pad = cfg.padding;

	draw_info_panel(pad, pad);

	const float content_top = pad + 190.0f;

	if (mode == ViewMode::Type)
	{
		spel_canvas_text_align_set(SPEL_CANVAS_ALIGN_LEFT);
		spel_canvas_font_set(loaded_font);
		spel_canvas_font_size_set(cfg.text_size);

		spel_vec2 sample_pos = v2(pad, content_top);
		spel_vec2 sample_size = spel_canvas_text_measure(edit_text.c_str());

		spel_canvas_fill_mode_set(SPEL_CANVAS_FILL);
		spel_canvas_color_set(spel_color_hexa(0x00FF0033));
		spel_canvas_draw_rect(
			recti(sample_pos.x, sample_pos.y, sample_size.x, sample_size.y));
		spel_canvas_color_set(spel_color_hexa(0xFF0000AA));
		spel_canvas_draw_rect(recti(sample_pos.x, sample_pos.y, 6, 6));

		if (cfg.show_metrics)
		{
			float boff = baseline_offset_y(loaded_font, cfg.text_size);
			float baseline_y = sample_pos.y + boff;

			float asc_y = baseline_y - (loaded_font->header.ascender * cfg.text_size);
			float desc_y = baseline_y - (loaded_font->header.descender * cfg.text_size);
			float lh_y = sample_pos.y + (loaded_font->header.line_height * cfg.text_size);

			spel_canvas_fill_mode_set(SPEL_CANVAS_STROKE);
			spel_canvas_line_width_set(1.0f);

			spel_canvas_color_set(spel_color_hexa(0x66CCFFFF));
			spel_canvas_draw_line(v2(sample_pos.x, baseline_y),
								  v2(sample_pos.x + sample_size.x, baseline_y));

			spel_canvas_color_set(spel_color_hexa(0xFFCC6666));
			spel_canvas_draw_line(v2(sample_pos.x, asc_y),
								  v2(sample_pos.x + sample_size.x, asc_y));

			spel_canvas_color_set(spel_color_hexa(0x66FFAA66));
			spel_canvas_draw_line(v2(sample_pos.x, desc_y),
								  v2(sample_pos.x + sample_size.x, desc_y));

			spel_canvas_color_set(spel_color_hexa(0xFF8888FF));
			spel_canvas_draw_line(v2(sample_pos.x, lh_y),
								  v2(sample_pos.x + sample_size.x, lh_y));

			spel_canvas_fill_mode_set(SPEL_CANVAS_FILL);
		}

		spel_canvas_color_set(spel_color_white);
		spel_canvas_draw_text(edit_text.c_str(), sample_pos);
	}
	else if (mode == ViewMode::Sheet &&
			 !sorted_glyphs.empty() && sheet_cell_w > 0.0f && sheet_cell_h > 0.0f)
	{
		float max_w = (float)spel.window.width - pad * 2.0f;
		int cols_fit = std::max(1, (int)std::floor(max_w / sheet_cell_w));
		int cols_draw = std::max(1, std::min(cfg.cols, cols_fit));

		float top = content_top;
		float max_y = (float)spel.window.height - pad;

		int start =
			std::clamp(sheet_first, 0, std::max(0, (int)sorted_glyphs.size() - 1));

		spel_canvas_fill_mode_set(SPEL_CANVAS_STROKE);
		spel_canvas_line_width_set(1.0f);

		int col = 0;
		int row = 0;

		for (int i = start; i < (int)sorted_glyphs.size(); i++)
		{
			const spel_font_glyph* g = sorted_glyphs[i];
			float x = pad + col * sheet_cell_w;
			float y = top + row * sheet_cell_h;

			if (y + sheet_cell_h > max_y)
				break;

			spel_canvas_color_set(spel_color_hexa(0xFFFFFF22));
			spel_canvas_draw_rect(recti(x, y, sheet_cell_w, sheet_cell_h));

			spel_canvas_fill_mode_set(SPEL_CANVAS_FILL);
			spel_canvas_font_set(loaded_font);
			spel_canvas_font_size_set(cfg.sheet_size);
			spel_canvas_color_set(spel_color_white);

			std::string ch = utf8(g->codepoint);
			spel_canvas_draw_text(ch.c_str(), v2(x + cfg.cell_pad, y + cfg.cell_pad));

			spel_canvas_font_set(ui_font);
			spel_canvas_font_size_set(12.0f);
			spel_canvas_color_set(spel_color_hexa(0xFFFFFFDD));
			if (g->codepoint <= 0xFFFF)
				spel_canvas_print(v2(x + 2, y + sheet_cell_h - 14), "U+%04X",
								  g->codepoint);
			else
				spel_canvas_print(v2(x + 2, y + sheet_cell_h - 14), "U+%06X",
								  g->codepoint);

			spel_canvas_fill_mode_set(SPEL_CANVAS_STROKE);

			col++;
			if (col >= cols_draw)
			{
				col = 0;
				row++;
			}
		}
	}
	else if (mode == ViewMode::Atlas)
	{
		float atlas_w = (float)loaded_font->header.atlas_width;
		float atlas_h = (float)loaded_font->header.atlas_height;

		float left = pad;
		float top = content_top;
		float max_w = (float)spel.window.width - pad - left;
		float max_h = (float)spel.window.height - pad - top;

		float s = std::min(max_w / atlas_w, max_h / atlas_h);
		if (s > 0.0f)
		{
			float w = atlas_w * s;
			float h = atlas_h * s;
			float x0 = left + (max_w - w) * 0.5f;
			float y0 = top + (max_h - h) * 0.5f;

			spel_rect dst = recti(x0, y0, w, h);
			spel_canvas_fill_mode_set(SPEL_CANVAS_FILL);
			spel_canvas_color_set(spel_color_white);
			spel_canvas_draw_image(loaded_font->atlas, dst);

			if (!sorted_glyphs.empty())
			{
				selected_glyph = std::clamp(selected_glyph, 0,
											std::max(0, (int)sorted_glyphs.size() - 1));
				const spel_font_glyph* g = sorted_glyphs[selected_glyph];

				float gx = x0 + g->uv_x * w;
				float gy = y0 + g->uv_y * h;
				float gw = g->uv_w * w;
				float gh = g->uv_h * h;

				spel_canvas_fill_mode_set(SPEL_CANVAS_STROKE);
				spel_canvas_line_width_set(2.0f);
				spel_canvas_color_set(spel_color_hexa(0xFF00FFCC));
				spel_canvas_draw_rect(recti(gx, gy, gw, gh));

				float info_y = cfg.show_help
								   ? (float)spel.window.height - pad - 120.0f - 22.0f
								   : (float)spel.window.height - pad - 22.0f;

				spel_canvas_fill_mode_set(SPEL_CANVAS_FILL);
				spel_canvas_font_set(ui_font);
				spel_canvas_font_size_set(16.0f);
				spel_canvas_color_set(spel_color_hexa(0xFFFFFFEE));
				spel_canvas_print(v2(pad, info_y),
								  "glyph %d/%d  U+%X  uv=(%.4f,%.4f %.4f,%.4f)",
								  selected_glyph + 1, (int)sorted_glyphs.size(),
								  g->codepoint, g->uv_x, g->uv_y, g->uv_w, g->uv_h);
			}
		}
	}

	if (cfg.show_help)
		draw_help_panel(pad, (float)spel.window.height - pad - 120.0f);

	spel_canvas_end();
}

extern "C" void spel_quit()
{
	if (loaded_font)
	{
		spel_font_destroy(loaded_font);
		loaded_font = nullptr;
	}
}
