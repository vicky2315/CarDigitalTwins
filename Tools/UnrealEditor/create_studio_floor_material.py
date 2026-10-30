"""Creates /Game/Twin/Studio/Materials/M_StudioFloor, the studio floor with the scrolling 1 m grid (build page step A3).

Run inside the Unreal editor (Python Editor Script Plugin), from the Output Log with the Cmd box:
    py "<project>/Tools/UnrealEditor/create_studio_floor_material.py"

Needs /Game/Twin/Studio/Materials/MPC_TwinStudio with the scalar GroundScrollCm (step A2) first; stops with an error if it's missing.
Never overwrites: if the material already exists it stops, so hand edits are safe. Delete the asset first to rebuild it.

Graph: Custom node StudioFloorGrid(WorldPos = Absolute World Position, ScrollCm = MPC GroundScrollCm) gives a 0-1 grid mask
that fades out 16 m from the origin; Emissive = grid colour x mask x 0.6; Base Color dark blue-grey, Roughness 0.45, Specular 0.3.
"""

import unreal

MATERIAL_FOLDER = "/Game/Twin/Studio/Materials"
MATERIAL_NAME = "M_StudioFloor"
STUDIO_COLLECTION_PATH = "/Game/Twin/Studio/Materials/MPC_TwinStudio"

# Must match the scalar AVehicleTwinActor writes the floor scroll into.
GROUND_SCROLL_PARAMETER = "GroundScrollCm"

GRID_LINE_COLOR_SRGB_HEX = "56B8DE"
FLOOR_BASE_COLOR_SRGB_HEX = "10171C"
GRID_EMISSIVE_STRENGTH = 0.6
FLOOR_ROUGHNESS = 0.45
FLOOR_SPECULAR = 0.3

# CellCm must match the car's Studio Floor Grid Cell Cm (100).
STUDIO_FLOOR_GRID_HLSL = """float CellCm = 100.0;
float LineHalfWidth = 1.0 / CellCm;
float2 GridPosition = (WorldPos.xy + float2(ScrollCm, 0.0)) / CellCm;
float2 DistanceFromLine = abs(frac(GridPosition) - 0.5);
float2 EdgeSoftness = fwidth(GridPosition);
float2 Lines = smoothstep(0.5 - LineHalfWidth - EdgeSoftness, 0.5 - LineHalfWidth, DistanceFromLine);
float GridMask = max(Lines.x, Lines.y);
float Fade = 1.0 - saturate(length(WorldPos.xy) / 1600.0);
return GridMask * Fade * Fade;"""

MaterialEditing = unreal.MaterialEditingLibrary
material_asset_path = f"{MATERIAL_FOLDER}/{MATERIAL_NAME}"


def add_node(material, expression_class, column, row):
    """Places a node on a grid so the graph stays readable when opened."""
    return MaterialEditing.create_material_expression(material, expression_class, -1600 + column * 260, row * 140)


def connect(from_node, from_output, to_node, to_input):
    """Wires one output to one input; "" means the first one. Raises if Unreal refuses, so a wrong name can't pass silently."""
    if not MaterialEditing.connect_material_expressions(from_node, from_output, to_node, to_input):
        raise RuntimeError(f"Could not connect {from_node.get_name()}.{from_output or 'first'} -> {to_node.get_name()}.{to_input or 'first'}")


def connect_to_material_output(from_node, material_property):
    if not MaterialEditing.connect_material_property(from_node, "", material_property):
        raise RuntimeError(f"Could not connect {from_node.get_name()} to {material_property}.")


def linear_color_from_srgb_hex(srgb_hex):
    """Same conversion as the colour picker's Hex sRGB field: the sRGB curve per channel, alpha 1."""
    def srgb_channel_to_linear(channel_byte):
        channel = channel_byte / 255.0
        return channel / 12.92 if channel <= 0.04045 else ((channel + 0.055) / 1.055) ** 2.4
    red, green, blue = (int(srgb_hex[index:index + 2], 16) for index in (0, 2, 4))
    return unreal.LinearColor(srgb_channel_to_linear(red), srgb_channel_to_linear(green), srgb_channel_to_linear(blue), 1.0)


def add_constant(material, value, column, row):
    constant_node = add_node(material, unreal.MaterialExpressionConstant, column, row)
    constant_node.set_editor_property("r", value)
    return constant_node


def add_constant_color(material, srgb_hex, column, row):
    color_node = add_node(material, unreal.MaterialExpressionConstant3Vector, column, row)
    color_node.set_editor_property("constant", linear_color_from_srgb_hex(srgb_hex))
    return color_node


def load_studio_collection():
    """MPC_TwinStudio from step A2, checked for the GroundScrollCm scalar."""
    studio_collection = unreal.EditorAssetLibrary.load_asset(STUDIO_COLLECTION_PATH)
    if studio_collection is None:
        raise RuntimeError(f"{STUDIO_COLLECTION_PATH} not found. Do build page step A2 first.")
    scalar_names = [str(scalar.get_editor_property("parameter_name")) for scalar in studio_collection.get_editor_property("scalar_parameters")]
    if GROUND_SCROLL_PARAMETER not in scalar_names:
        raise RuntimeError(f"{STUDIO_COLLECTION_PATH} has no scalar {GROUND_SCROLL_PARAMETER} (found: {scalar_names}).")
    return studio_collection


def create_studio_floor_material():
    if unreal.EditorAssetLibrary.does_asset_exist(material_asset_path):
        unreal.log_warning(f"{material_asset_path} already exists; nothing changed. Delete it first to rebuild it.")
        return

    studio_collection = load_studio_collection()

    material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        MATERIAL_NAME, MATERIAL_FOLDER, unreal.Material, unreal.MaterialFactoryNew())

    # Grid mask inputs. World Position's default output is Absolute World Position.
    absolute_world_position = add_node(material, unreal.MaterialExpressionWorldPosition, 0, 0)
    ground_scroll = add_node(material, unreal.MaterialExpressionCollectionParameter, 0, 1)
    ground_scroll.set_editor_property("collection", studio_collection)  # collection first: the name is looked up in it
    ground_scroll.set_editor_property("parameter_name", GROUND_SCROLL_PARAMETER)

    studio_floor_grid = add_node(material, unreal.MaterialExpressionCustom, 1, 0)
    studio_floor_grid.set_editor_property("description", "StudioFloorGrid")
    studio_floor_grid.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT1)
    grid_inputs = []
    for input_name in ("WorldPos", "ScrollCm"):
        grid_input = unreal.CustomInput()
        grid_input.set_editor_property("input_name", input_name)
        grid_inputs.append(grid_input)
    studio_floor_grid.set_editor_property("inputs", grid_inputs)
    studio_floor_grid.set_editor_property("code", STUDIO_FLOOR_GRID_HLSL)
    connect(absolute_world_position, "", studio_floor_grid, "WorldPos")
    connect(ground_scroll, "", studio_floor_grid, "ScrollCm")

    # Emissive = grid colour x mask x strength.
    grid_line_color = add_constant_color(material, GRID_LINE_COLOR_SRGB_HEX, 1, 2)
    grid_lines = add_node(material, unreal.MaterialExpressionMultiply, 2, 1)
    connect(grid_line_color, "", grid_lines, "A")
    connect(studio_floor_grid, "", grid_lines, "B")
    grid_emissive_strength = add_constant(material, GRID_EMISSIVE_STRENGTH, 2, 3)
    grid_emission = add_node(material, unreal.MaterialExpressionMultiply, 3, 2)
    connect(grid_lines, "", grid_emission, "A")
    connect(grid_emissive_strength, "", grid_emission, "B")
    connect_to_material_output(grid_emission, unreal.MaterialProperty.MP_EMISSIVE_COLOR)

    # The floor surface itself.
    connect_to_material_output(add_constant_color(material, FLOOR_BASE_COLOR_SRGB_HEX, 3, 4), unreal.MaterialProperty.MP_BASE_COLOR)
    connect_to_material_output(add_constant(material, FLOOR_ROUGHNESS, 3, 5), unreal.MaterialProperty.MP_ROUGHNESS)
    connect_to_material_output(add_constant(material, FLOOR_SPECULAR, 3, 6), unreal.MaterialProperty.MP_SPECULAR)

    MaterialEditing.layout_material_expressions(material)
    MaterialEditing.recompile_material(material)
    if not unreal.EditorAssetLibrary.save_asset(material_asset_path, only_if_is_dirty=False):
        raise RuntimeError(f"Could not save {material_asset_path} (read-only file or open elsewhere?): it exists only in memory. "
                           "MANUAL: File > Save All.")
    unreal.log(f"Created and saved {material_asset_path}.")
    unreal.log("MANUAL: assign it to StudioFloor > Materials > Element 0 (step A4), then save the level.")


create_studio_floor_material()
