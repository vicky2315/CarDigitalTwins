"""Rebuilds the vehicle twin Blueprint from the raw STEP file and vehicle_twin_recipe.json (written by export_vehicle_twin_recipe.py).

1. Datasmith import of the STEP with tessellation setting A (LEARNING_LOG Day 3) into REBUILD_IMPORT_FOLDER, assets only (no actors
   in the level). Skipped if that folder already has assets, so the Blueprint part can be rerun without the slow import.
2. A new Blueprint (parent AVehicleTwinActor) with the recipe's components: same names, hierarchy, transforms, tags, meshes and materials.
   Harvest's child actors become plain StaticMeshComponents (AVehicleTwinActor handles both).
3. Compares the result with the recipe and logs the counts.

Never overwrites: stops if the rebuilt Blueprint already exists. Writes only under /Game/Jeep/Rebuilt (gitignored with Content/Jeep/,
because the meshes come from a non-commercial CAD model: docs/ASSETS.md).

Run inside the Unreal editor, Output Log, Python mode:
    exec(open(r"E:\\Unreal Projects\\DigitalTwins\\CarDigitalTwins\\Tools\\UnrealEditor\\rebuild_vehicle_twin_from_cad.py").read())
"""

import json
import os

import unreal

REBUILD_IMPORT_FOLDER = "/Game/Jeep/Rebuilt/ImportA"
REBUILT_BLUEPRINT_FOLDER = "/Game/Jeep/Rebuilt"
REBUILT_BLUEPRINT_NAME = "BP_VehicleTwin_Rebuilt"

# Tessellation setting A (LEARNING_LOG Day 3, SPEC.md §7).
CHORD_TOLERANCE_CM = 0.2
MAX_EDGE_LENGTH_CM = 0.0
NORMAL_TOLERANCE_DEG = 20.0

project_folder = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
step_file_path = os.path.join(project_folder, "RawCAD", "2010 Jeep Wrangler Rubicon", "Imported CAD",
                              "2010 Jeep Wrangler Rubicon - Assembly.STEP")
recipe_file_path = os.path.join(project_folder, "Tools", "UnrealEditor", "vehicle_twin_recipe.json")
rebuilt_blueprint_path = f"{REBUILT_BLUEPRINT_FOLDER}/{REBUILT_BLUEPRINT_NAME}"

SubobjectLibrary = unreal.SubobjectDataBlueprintFunctionLibrary


def import_step_file():
    if unreal.EditorAssetLibrary.list_assets(REBUILD_IMPORT_FOLDER, recursive=True):
        unreal.log(f"{REBUILD_IMPORT_FOLDER} already has assets: import skipped. Delete the folder to import again.")
        return
    if not os.path.isfile(step_file_path):
        raise RuntimeError(f"STEP file not found: {step_file_path} (download steps: docs/ASSETS.md).")

    unreal.log(f"Importing {step_file_path}. The editor is busy for a few minutes.")
    datasmith_scene = unreal.DatasmithSceneElement.construct_datasmith_scene_from_file(step_file_path)
    if datasmith_scene is None:
        raise RuntimeError("Datasmith could not read the STEP file.")

    # Structs come back as copies: change them, then set them back.
    import_options = datasmith_scene.get_options(unreal.DatasmithImportOptions)
    base_options = import_options.get_editor_property("base_options")
    base_options.set_editor_property("scene_handling", unreal.DatasmithImportScene.ASSETS_ONLY)
    import_options.set_editor_property("base_options", base_options)

    tessellation_options_object = datasmith_scene.get_options(unreal.DatasmithCommonTessellationOptions)
    if tessellation_options_object is None:
        raise RuntimeError("No tessellation options: is the Datasmith CAD Importer plugin enabled?")
    tessellation_options = tessellation_options_object.get_editor_property("options")
    tessellation_options.set_editor_property("chord_tolerance", CHORD_TOLERANCE_CM)
    tessellation_options.set_editor_property("max_edge_length", MAX_EDGE_LENGTH_CM)
    tessellation_options.set_editor_property("normal_tolerance", NORMAL_TOLERANCE_DEG)
    tessellation_options.set_editor_property("stitching_technique", unreal.DatasmithCADStitchingTechnique.STITCHING_HEAL)
    tessellation_options_object.set_editor_property("options", tessellation_options)

    import_result = datasmith_scene.import_scene(REBUILD_IMPORT_FOLDER)
    if not import_result.get_editor_property("import_succeed"):
        raise RuntimeError("Datasmith import failed; see the Output Log above.")
    unreal.EditorAssetLibrary.save_directory(REBUILD_IMPORT_FOLDER)
    unreal.log(f"Imported {len(import_result.get_editor_property('imported_meshes'))} meshes into {REBUILD_IMPORT_FOLDER}.")


def load_recipe_asset(recipe_asset_path):
    """"import:<relative path>" points into the fresh import; other paths are used as they are."""
    if not recipe_asset_path:
        return None
    if recipe_asset_path.startswith("import:"):
        recipe_asset_path = f"{REBUILD_IMPORT_FOLDER}/{recipe_asset_path[len('import:'):]}"
    loaded_asset = unreal.load_asset(recipe_asset_path)
    if loaded_asset is None:
        unreal.log_warning(f"Asset not found: {recipe_asset_path}")
    return loaded_asset


def transform_from_recipe(recipe_transform):
    roll, pitch, yaw = recipe_transform["rotation_roll_pitch_yaw"]
    return unreal.Transform(unreal.Vector(*recipe_transform["location"]), unreal.Rotator(roll, pitch, yaw),
                            unreal.Vector(*recipe_transform["scale"]))


def is_identity_transform(recipe_transform):
    return (all(abs(value) < 1e-4 for value in recipe_transform["location"] + recipe_transform["rotation_roll_pitch_yaw"])
            and all(abs(value - 1.0) < 1e-4 for value in recipe_transform["scale"]))


def apply_recipe_to_component(component, recipe_component, missing_assets):
    component_transform = transform_from_recipe(recipe_component["transform"])
    child_mesh_transform = recipe_component.get("child_mesh_transform")
    if child_mesh_transform and not is_identity_transform(child_mesh_transform):
        # Harvest's child actor had its own root offset: apply it under the component's transform.
        component_transform = unreal.MathLibrary.compose_transforms(transform_from_recipe(child_mesh_transform), component_transform)

    component.set_editor_property("relative_location", component_transform.translation)
    component.set_editor_property("relative_rotation", component_transform.rotation.rotator())
    component.set_editor_property("relative_scale3d", component_transform.scale3d)
    component.set_editor_property("mobility", unreal.ComponentMobility.MOVABLE)
    component.set_editor_property("component_tags", [unreal.Name(tag) for tag in recipe_component["tags"]])

    if recipe_component["mesh"]:
        static_mesh = load_recipe_asset(recipe_component["mesh"])
        if static_mesh is None:
            missing_assets.append(recipe_component["mesh"])
            return
        component.set_editor_property("static_mesh", static_mesh)
        for slot_index, recipe_material_path in enumerate(recipe_component["materials"]):
            slot_material = load_recipe_asset(recipe_material_path)
            if slot_material is None:
                missing_assets.append(recipe_material_path)
            else:
                component.set_material(slot_index, slot_material)


def components_parents_first(recipe_components):
    """Orders components so every parent is created before its children."""
    recipe_component_by_name = {recipe_component["name"]: recipe_component for recipe_component in recipe_components}

    def depth(recipe_component):
        component_depth = 0
        while recipe_component["parent"] in recipe_component_by_name:
            recipe_component = recipe_component_by_name[recipe_component["parent"]]
            component_depth += 1
        return component_depth

    return sorted(recipe_components, key=depth)


def build_blueprint(recipe):
    blueprint_factory = unreal.BlueprintFactory()
    blueprint_factory.set_editor_property("parent_class", unreal.VehicleTwinActor)
    blueprint = unreal.AssetToolsHelpers.get_asset_tools().create_asset(REBUILT_BLUEPRINT_NAME, REBUILT_BLUEPRINT_FOLDER, unreal.Blueprint,
                                                                       blueprint_factory)
    subobject_subsystem = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)

    # The new Blueprint has a DefaultSceneRoot: the recipe's root components go under it (it sits at 0,0,0, so nothing moves).
    default_scene_root_handle = None
    for subobject_handle in subobject_subsystem.k2_gather_subobject_data_for_blueprint(blueprint):
        if SubobjectLibrary.is_root_component(SubobjectLibrary.get_data(subobject_handle)):
            default_scene_root_handle = subobject_handle
    if default_scene_root_handle is None:
        raise RuntimeError("New Blueprint has no root component.")

    handle_by_component_name = {}
    missing_assets = []
    for recipe_component in components_parents_first(recipe["components"]):
        if recipe_component["parent"] is None and recipe_component["name"] == "DefaultSceneRoot" and not recipe_component["mesh"]:
            # The hand-built Blueprint's own default root: reuse the new one instead of adding a second.
            handle_by_component_name[recipe_component["name"]] = default_scene_root_handle
            continue

        parent_handle = handle_by_component_name.get(recipe_component["parent"], default_scene_root_handle)
        component_class = unreal.StaticMeshComponent if recipe_component["mesh"] else unreal.SceneComponent
        new_subobject_params = unreal.AddNewSubobjectParams(parent_handle=parent_handle, new_class=component_class,
                                                            blueprint_context=blueprint)
        new_handle, fail_reason = subobject_subsystem.add_new_subobject(new_subobject_params)
        if not SubobjectLibrary.is_handle_valid(new_handle):
            raise RuntimeError(f"Could not add {recipe_component['name']}: {fail_reason}")
        if not subobject_subsystem.rename_subobject(new_handle, unreal.Text(recipe_component["name"])):
            unreal.log_warning(f"Could not name a component {recipe_component['name']}; it keeps its default name.")
        handle_by_component_name[recipe_component["name"]] = new_handle

        component = SubobjectLibrary.get_object(SubobjectLibrary.get_data(new_handle))
        apply_recipe_to_component(component, recipe_component, missing_assets)

    unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)

    class_defaults = unreal.get_default_object(blueprint.generated_class())
    status_outline_material = load_recipe_asset(recipe["class_defaults"].get("status_outline_material"))
    if status_outline_material is not None:
        class_defaults.set_editor_property("status_outline_material", status_outline_material)

    unreal.EditorAssetLibrary.save_asset(rebuilt_blueprint_path, only_if_is_dirty=False)
    return blueprint, missing_assets


def compare_with_recipe(blueprint, recipe, missing_assets):
    subobject_subsystem = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
    rebuilt_tag_counts, rebuilt_meshes_count = {}, 0
    counted_object_paths = set()
    for subobject_handle in subobject_subsystem.k2_gather_subobject_data_for_blueprint(blueprint):
        component = SubobjectLibrary.get_object(SubobjectLibrary.get_data(subobject_handle))
        # The gather returns every component twice in 5.7: count each once.
        if component is None or component.get_path_name() in counted_object_paths:
            continue
        counted_object_paths.add(component.get_path_name())
        if isinstance(component, unreal.StaticMeshComponent) and component.get_editor_property("static_mesh") is not None:
            rebuilt_meshes_count += 1
        if isinstance(component, unreal.SceneComponent):
            for tag in component.get_editor_property("component_tags"):
                rebuilt_tag_counts[str(tag)] = rebuilt_tag_counts.get(str(tag), 0) + 1

    recipe_tag_counts = {}
    for recipe_component in recipe["components"]:
        for tag in recipe_component["tags"]:
            recipe_tag_counts[tag] = recipe_tag_counts.get(tag, 0) + 1
    recipe_meshes_count = sum(1 for recipe_component in recipe["components"] if recipe_component["mesh"])

    # Class defaults aren't components, so check the one the outline needs separately.
    rebuilt_outline_material = unreal.get_default_object(blueprint.generated_class()).get_editor_property("status_outline_material")
    recipe_outline_material_path = recipe["class_defaults"].get("status_outline_material")
    outline_material_matches = (rebuilt_outline_material is not None) == bool(recipe_outline_material_path)
    unreal.log(f"Status Outline Material: recipe {recipe_outline_material_path}, rebuilt "
               f"{rebuilt_outline_material.get_path_name() if rebuilt_outline_material else None}")

    unreal.log(f"Recipe:  {recipe_meshes_count} meshes, tags {dict(sorted(recipe_tag_counts.items()))}")
    unreal.log(f"Rebuilt: {rebuilt_meshes_count} meshes, tags {dict(sorted(rebuilt_tag_counts.items()))}")
    if missing_assets:
        unreal.log_warning(f"{len(missing_assets)} assets missing from the import (first 10): {missing_assets[:10]}")
    if (recipe_meshes_count == rebuilt_meshes_count and recipe_tag_counts == rebuilt_tag_counts and not missing_assets
            and outline_material_matches):
        unreal.log(f"{rebuilt_blueprint_path} matches the recipe.")
    else:
        unreal.log_warning(f"{rebuilt_blueprint_path} does NOT match the recipe; see the lines above.")


def rebuild_vehicle_twin_from_cad():
    if unreal.EditorAssetLibrary.does_asset_exist(rebuilt_blueprint_path):
        unreal.log_warning(f"{rebuilt_blueprint_path} already exists; nothing changed. Delete it first to rebuild.")
        return
    if not os.path.isfile(recipe_file_path):
        raise RuntimeError(f"{recipe_file_path} not found: run export_vehicle_twin_recipe.py first.")
    with open(recipe_file_path, encoding="utf-8") as recipe_file:
        recipe = json.load(recipe_file)

    import_step_file()
    blueprint, missing_assets = build_blueprint(recipe)
    compare_with_recipe(blueprint, recipe, missing_assets)


rebuild_vehicle_twin_from_cad()
