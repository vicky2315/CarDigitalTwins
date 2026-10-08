"""Writes vehicle_twin_recipe.json from the hand-built BP_VehicleTwin: every component's name, parent, transform, tags, mesh and
materials. rebuild_vehicle_twin_from_cad.py uses it to rebuild the car from the STEP file.

The recipe holds only names, paths, tags and transforms (no geometry), so it can live in the public repo.
Mesh and material paths under the Datasmith import folder are stored relative to it, so the rebuild can point them at a fresh import.

Run inside the Unreal editor, Output Log, Python mode:
    exec(open(r"E:\\Unreal Projects\\DigitalTwins\\CarDigitalTwins\\Tools\\UnrealEditor\\export_vehicle_twin_recipe.py").read())
Read-only: changes no asset.
"""

import json
import os

import unreal

SOURCE_BLUEPRINT_PATH = "/Game/Jeep/Blueprints/BP_VehicleTwin"
SOURCE_IMPORT_FOLDER = "/Game/Jeep/ImportA"

project_folder = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
recipe_file_path = os.path.join(project_folder, "Tools", "UnrealEditor", "vehicle_twin_recipe.json")

SubobjectLibrary = unreal.SubobjectDataBlueprintFunctionLibrary


def asset_path_for_recipe(asset):
    """Path inside the import folder becomes "import:<relative path>"; anything else (e.g. /Game/Materials) stays absolute."""
    if asset is None:
        return None
    asset_path = asset.get_path_name().split(".")[0]
    if asset_path.startswith(SOURCE_IMPORT_FOLDER + "/"):
        return "import:" + asset_path[len(SOURCE_IMPORT_FOLDER) + 1:]
    return asset_path


def transform_for_recipe(location, rotation, scale):
    return {
        "location": [location.x, location.y, location.z],
        "rotation_roll_pitch_yaw": [rotation.roll, rotation.pitch, rotation.yaw],
        "scale": [scale.x, scale.y, scale.z],
    }


def component_transform_for_recipe(component):
    return transform_for_recipe(component.get_editor_property("relative_location"), component.get_editor_property("relative_rotation"),
                                component.get_editor_property("relative_scale3d"))


def mesh_and_materials_for_recipe(static_mesh_component):
    static_mesh = static_mesh_component.get_editor_property("static_mesh")
    slot_materials = [asset_path_for_recipe(static_mesh_component.get_material(slot_index))
                      for slot_index in range(static_mesh_component.get_num_materials())]
    return asset_path_for_recipe(static_mesh), slot_materials


def export_vehicle_twin_recipe():
    blueprint = unreal.load_asset(SOURCE_BLUEPRINT_PATH)
    if blueprint is None:
        raise RuntimeError(f"{SOURCE_BLUEPRINT_PATH} not found.")

    subobject_subsystem = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
    subobject_handles = subobject_subsystem.k2_gather_subobject_data_for_blueprint(blueprint)

    # Variable name of every component first, keyed by the component template's path, so parents can be named.
    variable_name_by_object_path = {}
    for subobject_handle in subobject_handles:
        subobject_data = SubobjectLibrary.get_data(subobject_handle)
        subobject = SubobjectLibrary.get_object(subobject_data)
        if subobject is not None and not SubobjectLibrary.is_actor(subobject_data):
            variable_name_by_object_path[subobject.get_path_name()] = str(SubobjectLibrary.get_variable_name(subobject_data))

    recipe_components = []
    exported_object_paths = set()
    for subobject_handle in subobject_handles:
        subobject_data = SubobjectLibrary.get_data(subobject_handle)
        if SubobjectLibrary.is_actor(subobject_data):
            continue
        component = SubobjectLibrary.get_object(subobject_data)
        if not isinstance(component, unreal.SceneComponent):
            continue
        # The gather returns every component twice in 5.7: export each once.
        if component.get_path_name() in exported_object_paths:
            continue
        exported_object_paths.add(component.get_path_name())

        # None for the root (its parent is the actor itself).
        parent_handle = SubobjectLibrary.get_parent_handle(subobject_data)
        parent_object = SubobjectLibrary.get_object(SubobjectLibrary.get_data(parent_handle)) if SubobjectLibrary.is_handle_valid(parent_handle) else None
        parent_name = variable_name_by_object_path.get(parent_object.get_path_name()) if parent_object is not None else None

        recipe_component = {
            "name": variable_name_by_object_path[component.get_path_name()],
            "parent": parent_name,
            "tags": [str(tag) for tag in component.get_editor_property("component_tags")],
            "transform": component_transform_for_recipe(component),
            "mesh": None,
            "materials": [],
        }

        if isinstance(component, unreal.StaticMeshComponent):
            recipe_component["mesh"], recipe_component["materials"] = mesh_and_materials_for_recipe(component)
        elif isinstance(component, unreal.ChildActorComponent):
            # Harvest Components: the mesh sits on the child actor template's root component.
            child_actor_template = component.get_editor_property("child_actor_template")
            child_mesh_component = child_actor_template.get_editor_property("static_mesh_component") if isinstance(
                child_actor_template, unreal.StaticMeshActor) else None
            if child_mesh_component is None:
                unreal.log_warning(f"{recipe_component['name']}: child actor without a static mesh, kept as an empty scene component.")
            else:
                recipe_component["mesh"], recipe_component["materials"] = mesh_and_materials_for_recipe(child_mesh_component)
                # The child actor's own root offset, applied under the component's transform when rebuilding.
                recipe_component["child_mesh_transform"] = component_transform_for_recipe(child_mesh_component)

        recipe_components.append(recipe_component)

    class_defaults = unreal.get_default_object(blueprint.generated_class())
    status_outline_material = class_defaults.get_editor_property("status_outline_material")

    # FVehicleTwinPaintGroup isn't a BlueprintType struct, so Python may not see it. Informational only: the rebuild keeps the C++
    # defaults (Paint 1, Trim 1).
    try:
        paint_groups = [{"component_tag": str(paint_group.get_editor_property("component_tag")),
                         "status_blend_scale": paint_group.get_editor_property("status_blend_scale")}
                        for paint_group in class_defaults.get_editor_property("paint_groups")]
    except Exception as paint_groups_error:
        unreal.log_warning(f"Paint groups not readable from Python ({paint_groups_error}); not stored in the recipe.")
        paint_groups = None

    recipe = {
        "source_blueprint": SOURCE_BLUEPRINT_PATH,
        "source_import_folder": SOURCE_IMPORT_FOLDER,
        "class_defaults": {
            "status_outline_material": asset_path_for_recipe(status_outline_material),
            "paint_groups": paint_groups,
        },
        "components": recipe_components,
    }

    with open(recipe_file_path, "w", encoding="utf-8") as recipe_file:
        json.dump(recipe, recipe_file, indent=2)

    tag_counts = {}
    for recipe_component in recipe_components:
        for tag in recipe_component["tags"]:
            tag_counts[tag] = tag_counts.get(tag, 0) + 1
    meshes_count = sum(1 for recipe_component in recipe_components if recipe_component["mesh"])
    unreal.log(f"Recipe written: {recipe_file_path}")
    unreal.log(f"  {len(recipe_components)} components, {meshes_count} with a mesh. Tags: {dict(sorted(tag_counts.items()))}")


export_vehicle_twin_recipe()
