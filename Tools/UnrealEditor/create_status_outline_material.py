"""Creates /Game/Materials/PP_VehicleStatusOutline, the post-process material behind AVehicleTwinActor's status outline (SPEC.md §1).

Run inside the Unreal editor (Python Editor Script Plugin), from the Output Log with the Cmd box:
    py "<project>/Tools/UnrealEditor/create_status_outline_material.py"

Never overwrites: if the material already exists it stops, so hand edits are safe. Delete the asset first to rebuild it.

Graph: four CustomStencil taps StatusOutlineWidthPixels apart (left, right, up, down) find pixels next to the car;
outline = neighbour stencil x (1 - centre stencil); Emissive = scene colour + outline x StatusOutlineColor x StatusOutlineGlowIntensity.
"""

import unreal

MATERIAL_FOLDER = "/Game/Materials"
MATERIAL_NAME = "PP_VehicleStatusOutline"

# Must match AVehicleTwinActor::StatusOutlineColorParameter / StatusOutlineGlowIntensityParameter.
OUTLINE_COLOR_PARAMETER = "StatusOutlineColor"
OUTLINE_GLOW_INTENSITY_PARAMETER = "StatusOutlineGlowIntensity"
OUTLINE_WIDTH_PIXELS_PARAMETER = "StatusOutlineWidthPixels"

MaterialEditing = unreal.MaterialEditingLibrary
material_asset_path = f"{MATERIAL_FOLDER}/{MATERIAL_NAME}"


def add_node(material, expression_class, column, row):
    """Places a node on a grid so the graph stays readable when opened."""
    return MaterialEditing.create_material_expression(material, expression_class, -1600 + column * 260, row * 140)


def connect(from_node, from_output, to_node, to_input):
    """Wires one output to one input; "" means the first one. Raises if Unreal refuses, so a wrong name can't pass silently."""
    if not MaterialEditing.connect_material_expressions(from_node, from_output, to_node, to_input):
        raise RuntimeError(f"Could not connect {from_node.get_name()}.{from_output or 'first'} -> {to_node.get_name()}.{to_input or 'first'}")


def add_custom_stencil_tap(material, uv_source_node, column, row):
    """CustomStencil sampled at uv_source_node's output, reduced to its R channel (the stencil value)."""
    stencil_tap = add_node(material, unreal.MaterialExpressionSceneTexture, column, row)
    stencil_tap.set_editor_property("scene_texture_id", unreal.SceneTextureId.PPI_CUSTOM_STENCIL)
    if uv_source_node is not None:
        connect(uv_source_node, "", stencil_tap, "")
    stencil_value = add_node(material, unreal.MaterialExpressionComponentMask, column + 1, row)
    stencil_value.set_editor_property("r", True)
    stencil_value.set_editor_property("g", False)
    stencil_value.set_editor_property("b", False)
    stencil_value.set_editor_property("a", False)
    connect(stencil_tap, "Color", stencil_value, "")
    return stencil_value


def create_status_outline_material():
    if unreal.EditorAssetLibrary.does_asset_exist(material_asset_path):
        unreal.log_warning(f"{material_asset_path} already exists; nothing changed. Delete it first to rebuild it.")
        return

    material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        MATERIAL_NAME, MATERIAL_FOLDER, unreal.Material, unreal.MaterialFactoryNew())
    material.set_editor_property("material_domain", unreal.MaterialDomain.MD_POST_PROCESS)
    # Before bloom, so an intensity above 1 glows.
    material.set_editor_property("blendable_location", unreal.BlendableLocation.BL_SCENE_COLOR_BEFORE_BLOOM)

    # Screen UV and the per-pixel offset (one texel x width in pixels).
    screen_uv = add_node(material, unreal.MaterialExpressionScreenPosition, 0, 0)
    scene_color = add_node(material, unreal.MaterialExpressionSceneTexture, 0, 2)
    scene_color.set_editor_property("scene_texture_id", unreal.SceneTextureId.PPI_POST_PROCESS_INPUT0)

    outline_width_pixels = add_node(material, unreal.MaterialExpressionScalarParameter, 0, 3)
    outline_width_pixels.set_editor_property("parameter_name", OUTLINE_WIDTH_PIXELS_PARAMETER)
    outline_width_pixels.set_editor_property("default_value", 3.0)

    tap_offset = add_node(material, unreal.MaterialExpressionMultiply, 1, 2)
    connect(scene_color, "InvSize", tap_offset, "A")
    connect(outline_width_pixels, "", tap_offset, "B")

    horizontal_axis = add_node(material, unreal.MaterialExpressionConstant2Vector, 1, 4)
    horizontal_axis.set_editor_property("r", 1.0)
    horizontal_axis.set_editor_property("g", 0.0)
    vertical_axis = add_node(material, unreal.MaterialExpressionConstant2Vector, 1, 5)
    vertical_axis.set_editor_property("r", 0.0)
    vertical_axis.set_editor_property("g", 1.0)

    horizontal_offset = add_node(material, unreal.MaterialExpressionMultiply, 2, 3)
    connect(tap_offset, "", horizontal_offset, "A")
    connect(horizontal_axis, "", horizontal_offset, "B")
    vertical_offset = add_node(material, unreal.MaterialExpressionMultiply, 2, 5)
    connect(tap_offset, "", vertical_offset, "A")
    connect(vertical_axis, "", vertical_offset, "B")

    # The four neighbour UVs: right, left, down, up.
    neighbour_uvs = []
    for row, (expression_class, offset_node) in enumerate([
            (unreal.MaterialExpressionAdd, horizontal_offset),
            (unreal.MaterialExpressionSubtract, horizontal_offset),
            (unreal.MaterialExpressionAdd, vertical_offset),
            (unreal.MaterialExpressionSubtract, vertical_offset)]):
        neighbour_uv = add_node(material, expression_class, 3, row * 2)
        connect(screen_uv, "ViewportUV", neighbour_uv, "A")
        connect(offset_node, "", neighbour_uv, "B")
        neighbour_uvs.append(neighbour_uv)

    neighbour_stencils = [add_custom_stencil_tap(material, neighbour_uv, 4, row * 2) for row, neighbour_uv in enumerate(neighbour_uvs)]

    # Largest of the four neighbours: > 0 when any of them is on the car.
    horizontal_neighbour_max = add_node(material, unreal.MaterialExpressionMax, 6, 1)
    connect(neighbour_stencils[0], "", horizontal_neighbour_max, "A")
    connect(neighbour_stencils[1], "", horizontal_neighbour_max, "B")
    vertical_neighbour_max = add_node(material, unreal.MaterialExpressionMax, 6, 5)
    connect(neighbour_stencils[2], "", vertical_neighbour_max, "A")
    connect(neighbour_stencils[3], "", vertical_neighbour_max, "B")
    any_neighbour_max = add_node(material, unreal.MaterialExpressionMax, 7, 3)
    connect(horizontal_neighbour_max, "", any_neighbour_max, "A")
    connect(vertical_neighbour_max, "", any_neighbour_max, "B")
    neighbour_on_car = add_node(material, unreal.MaterialExpressionSaturate, 8, 3)
    connect(any_neighbour_max, "", neighbour_on_car, "")

    # The pixel itself: on the car = no outline there (the outline sits just outside the silhouette).
    centre_stencil = add_custom_stencil_tap(material, screen_uv, 4, 9)
    centre_on_car = add_node(material, unreal.MaterialExpressionSaturate, 6, 9)
    connect(centre_stencil, "", centre_on_car, "")
    centre_off_car = add_node(material, unreal.MaterialExpressionOneMinus, 7, 9)
    connect(centre_on_car, "", centre_off_car, "")

    outline_mask = add_node(material, unreal.MaterialExpressionMultiply, 9, 5)
    connect(neighbour_on_car, "", outline_mask, "A")
    connect(centre_off_car, "", outline_mask, "B")

    # Colour x glow, both set every frame by AVehicleTwinActor.
    outline_color = add_node(material, unreal.MaterialExpressionVectorParameter, 8, 7)
    outline_color.set_editor_property("parameter_name", OUTLINE_COLOR_PARAMETER)
    outline_color.set_editor_property("default_value", unreal.LinearColor(1.0, 0.45, 0.0, 1.0))
    outline_glow_intensity = add_node(material, unreal.MaterialExpressionScalarParameter, 8, 8)
    outline_glow_intensity.set_editor_property("parameter_name", OUTLINE_GLOW_INTENSITY_PARAMETER)
    outline_glow_intensity.set_editor_property("default_value", 0.0)

    outline_color_rgb = add_node(material, unreal.MaterialExpressionComponentMask, 9, 7)
    for channel, enabled in (("r", True), ("g", True), ("b", True), ("a", False)):
        outline_color_rgb.set_editor_property(channel, enabled)
    connect(outline_color, "", outline_color_rgb, "")

    glowing_outline_color = add_node(material, unreal.MaterialExpressionMultiply, 10, 7)
    connect(outline_color_rgb, "", glowing_outline_color, "A")
    connect(outline_glow_intensity, "", glowing_outline_color, "B")
    outline_emission = add_node(material, unreal.MaterialExpressionMultiply, 11, 5)
    connect(outline_mask, "", outline_emission, "A")
    connect(glowing_outline_color, "", outline_emission, "B")

    # Scene colour (RGB) plus the outline.
    scene_color_rgb = add_node(material, unreal.MaterialExpressionComponentMask, 1, 0)
    for channel, enabled in (("r", True), ("g", True), ("b", True), ("a", False)):
        scene_color_rgb.set_editor_property(channel, enabled)
    connect(scene_color, "Color", scene_color_rgb, "")
    final_color = add_node(material, unreal.MaterialExpressionAdd, 12, 3)
    connect(scene_color_rgb, "", final_color, "A")
    connect(outline_emission, "", final_color, "B")

    if not MaterialEditing.connect_material_property(final_color, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR):
        raise RuntimeError("Could not connect the result to Emissive Color.")

    MaterialEditing.layout_material_expressions(material)
    MaterialEditing.recompile_material(material)
    unreal.EditorAssetLibrary.save_asset(material_asset_path, only_if_is_dirty=False)
    unreal.log(f"Created {material_asset_path}. Assign it to BP_VehicleTwin > Vehicle Twin | Status Outline > Status Outline Material.")


create_status_outline_material()
