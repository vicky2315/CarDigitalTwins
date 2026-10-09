"""Draws the dashboard's icon textures as white-on-transparent PNGs in Art/UI/Icons/ (import them into /Game/UI/Icons/).

White so UMG can tint them (view icons grey, the alert triangle amber or red). Shapes follow the SVG icons of the Twin Ops Dashboard
mock, drawn on a 24-unit grid at 4x and scaled down for smooth edges. Needs Pillow (pip install pillow). Run from the project folder:
    python Tools/UnrealEditor/generate_dashboard_icons.py
"""

from pathlib import Path

from PIL import Image, ImageDraw

OUTPUT_FOLDER = Path(__file__).resolve().parents[2] / "Art" / "UI" / "Icons"
ICON_SIZE_PX = 64
SUPERSAMPLE = 4
WHITE = (255, 255, 255, 255)


def new_canvas(size_px):
    canvas_px = size_px * SUPERSAMPLE
    return Image.new("RGBA", (canvas_px, canvas_px), (0, 0, 0, 0)), canvas_px / 24.0


def grid_points(scale, points):
    return [(x * scale, y * scale) for x, y in points]


def draw_polyline(draw, scale, points, stroke_units=2.0, closed=False):
    if closed:
        points = points + [points[0]]
    width = round(stroke_units * scale)
    draw.line(grid_points(scale, points), fill=WHITE, width=width, joint="curve")
    # Round caps and corners: a dot at every point.
    radius = width / 2
    for x, y in grid_points(scale, points):
        draw.ellipse((x - radius, y - radius, x + radius, y + radius), fill=WHITE)


def draw_ring(draw, scale, centre, radius_units, stroke_units=2.0):
    cx, cy = centre[0] * scale, centre[1] * scale
    r = radius_units * scale
    draw.ellipse((cx - r, cy - r, cx + r, cy + r), outline=WHITE, width=round(stroke_units * scale))


def save(image, size_px, name):
    OUTPUT_FOLDER.mkdir(parents=True, exist_ok=True)
    image.resize((size_px, size_px), Image.LANCZOS).save(OUTPUT_FOLDER / f"{name}.png")
    print(f"{name}.png")


def view_overview():
    image, s = new_canvas(ICON_SIZE_PX)
    draw = ImageDraw.Draw(image)
    width = round(2 * s)
    draw.arc((4 * s, 5 * s, 20 * s, 21 * s), start=180, end=360, fill=WHITE, width=width)   # gauge: half circle, centre (12, 13), r 8
    draw_polyline(draw, s, [(12, 13), (16, 9)])                                                # needle
    draw.ellipse((10.4 * s, 11.4 * s, 13.6 * s, 14.6 * s), fill=WHITE)                       # hub
    save(image, ICON_SIZE_PX, "T_Icon_ViewOverview")


def view_powertrain():
    image, s = new_canvas(ICON_SIZE_PX)
    draw = ImageDraw.Draw(image)
    draw_polyline(draw, s, [(6, 8), (9, 8), (10, 6), (14, 6), (15, 8), (18, 8), (18, 17), (6, 17)], closed=True)  # engine block
    draw_polyline(draw, s, [(3, 11), (3, 14)])
    draw_polyline(draw, s, [(21, 11), (21, 14)])
    save(image, ICON_SIZE_PX, "T_Icon_ViewPowertrain")


def view_tyres():
    image, s = new_canvas(ICON_SIZE_PX)
    draw = ImageDraw.Draw(image)
    draw_ring(draw, s, (12, 12), 8)
    draw_ring(draw, s, (12, 12), 3)
    save(image, ICON_SIZE_PX, "T_Icon_ViewTyres")


def view_body():
    image, s = new_canvas(ICON_SIZE_PX)
    draw = ImageDraw.Draw(image)
    draw_polyline(draw, s, [(3, 15), (5, 10), (19, 10), (21, 15), (21, 18), (3, 18)], closed=True)  # body
    draw_polyline(draw, s, [(8, 10), (9.5, 7), (14.5, 7), (16, 10)])                                 # cabin
    save(image, ICON_SIZE_PX, "T_Icon_ViewBody")


def status_ok():
    image, s = new_canvas(ICON_SIZE_PX)
    draw = ImageDraw.Draw(image)
    draw_ring(draw, s, (12, 12), 9)
    draw_polyline(draw, s, [(7.5, 12.5), (10.5, 15.5), (16.5, 9)])
    save(image, ICON_SIZE_PX, "T_Icon_StatusOk")


def status_alert():
    # A filled triangle with the exclamation mark cut out, so the tint colours the triangle and the mark shows the panel behind it.
    image, s = new_canvas(ICON_SIZE_PX)
    draw = ImageDraw.Draw(image)
    draw.polygon(grid_points(s, [(12, 2), (23, 21), (1, 21)]), fill=WHITE)
    clear = (0, 0, 0, 0)
    draw.rectangle((11 * s, 9 * s, 13 * s, 15 * s), fill=clear)
    draw.rectangle((11 * s, 16.5 * s, 13 * s, 18.5 * s), fill=clear)
    save(image, ICON_SIZE_PX, "T_Icon_StatusAlert")


def part_ring():
    # 128 px ring for WBP_PartRing: shown at 92 px with a 4 px edge, as in the mock.
    size_px = 128
    canvas_px = size_px * SUPERSAMPLE
    image = Image.new("RGBA", (canvas_px, canvas_px), (0, 0, 0, 0))
    draw = ImageDraw.Draw(image)
    edge_px = round(4 * (size_px / 92) * SUPERSAMPLE)
    inset = edge_px // 2 + SUPERSAMPLE
    draw.ellipse((inset, inset, canvas_px - inset, canvas_px - inset), outline=WHITE, width=edge_px)
    save(image, size_px, "T_PartRing")


if __name__ == "__main__":
    for draw_icon in (view_overview, view_powertrain, view_tyres, view_body, status_ok, status_alert, part_ring):
        draw_icon()
    print(f"Written to {OUTPUT_FOLDER}")