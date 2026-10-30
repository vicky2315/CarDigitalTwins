"""Adds the three callout anchor Scene Components to BP_VehicleTwin (build page step A7).

Run inside the Unreal editor (Python Editor Script Plugin), from the Output Log with the Cmd box:
    py "<project>/Tools/UnrealEditor/create_callout_anchors.py"
Close BP_VehicleTwin's editor tab first, so it doesn't show a stale component list.

Anchors are found by component name: missing ones are added under the root component, existing ones get their tag again.
AVehicleTwinActor::FindCalloutAnchorWorldLocation finds an anchor by its Component Tag, so the tag is what matters.
Locations are starting points in actor space (car faces +X). Fine-tune with the move gizmo in the Blueprint viewport afterwards:
a re-run keeps those hand-placed locations unless RESET_EXISTING_LOCATIONS is True.
Compiles and saves the Blueprint at the end.
"""

import unreal

BLUEPRINT_PATH = "/Game/Jeep/Blueprints/BP_VehicleTwin"
RESET_EXISTING_LOCATIONS = False

# Location (cm) relative to the actor.
CALLOUT_ANCHORS = [
    {"name": "CalloutAnchor_Hood",   "tag": "CalloutAnchor.Hood",           "location": (150, 0, 115)},   # top of the hood, middle
    {"name": "CalloutAnchor_Engine", "tag": "CalloutAnchor.Engine",         "location": (60, 0, 150)},    # base of the windscreen
    {"name": "CalloutAnchor_DoorFL", "tag": "CalloutAnchor.DoorFrontLeft",  "location": (40, -95, 100)},  # front-left door, outer face
]

SubobjectLibrary = unreal.SubobjectDataBlueprintFunctionLibrary


def set_properties(target, properties):
    """set_editor_property per entry; names the property that failed instead of a bare exception."""
    for name, value in properties.items():
        try:
            target.set_editor_property(name, value)
        except Exception as error:
            raise RuntimeError(f"Could not set {name} on {target.get_name()}: {error}") from error


def gather_components_by_name(subobject_subsystem, blueprint):
    """Variable name -> (handle, component), plus the root's handle. The gather returns every component twice (5.7): skip repeats."""
    handle_and_component_by_name = {}
    root_component_handle = None
    seen_object_paths = set()
    for subobject_handle in subobject_subsystem.k2_gather_subobject_data_for_blueprint(blueprint):
        subobject_data = SubobjectLibrary.get_data(subobject_handle)
        if SubobjectLibrary.is_actor(subobject_data):
            continue
        component = SubobjectLibrary.get_object(subobject_data)
        if component is None or component.get_path_name() in seen_object_paths:
            continue
        seen_object_paths.add(component.get_path_name())
        if root_component_handle is None and SubobjectLibrary.is_root_component(subobject_data):
            root_component_handle = subobject_handle
        handle_and_component_by_name[str(SubobjectLibrary.get_variable_name(subobject_data))] = (subobject_handle, component)
    return handle_and_component_by_name, root_component_handle


def add_scene_component(subobject_subsystem, blueprint, parent_handle, component_name):
    new_subobject_params = unreal.AddNewSubobjectParams(parent_handle=parent_handle, new_class=unreal.SceneComponent,
                                                        blueprint_context=blueprint)
    new_handle, fail_reason = subobject_subsystem.add_new_subobject(new_subobject_params)
    if not SubobjectLibrary.is_handle_valid(new_handle):
        raise RuntimeError(f"Could not add {component_name}: {fail_reason}")
    if not subobject_subsystem.rename_subobject(new_handle, unreal.Text(component_name)):
        raise RuntimeError(f"Added a Scene Component but could not name it {component_name}: rename or delete it by hand.")
    return SubobjectLibrary.get_object(SubobjectLibrary.get_data(new_handle))


def create_callout_anchors():
    blueprint = unreal.load_asset(BLUEPRINT_PATH)
    if blueprint is None:
        raise RuntimeError(f"{BLUEPRINT_PATH} not found. It is local-only (Content/Jeep/): run this on the personal laptop.")

    subobject_subsystem = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
    handle_and_component_by_name, root_component_handle = gather_components_by_name(subobject_subsystem, blueprint)
    if root_component_handle is None:
        raise RuntimeError(f"{BLUEPRINT_PATH} has no root component.")

    for spec in CALLOUT_ANCHORS:
        existing = handle_and_component_by_name.get(spec["name"])
        if existing is None:
            anchor_component = add_scene_component(subobject_subsystem, blueprint, root_component_handle, spec["name"])
            set_location = True
        else:
            anchor_component = existing[1]
            set_location = RESET_EXISTING_LOCATIONS

        # The tag list is replaced, not appended to, so a re-run can't leave a stale tag behind.
        set_properties(anchor_component, {"component_tags": [unreal.Name(spec["tag"])]})
        if set_location:
            set_properties(anchor_component, {"relative_location": unreal.Vector(*spec["location"])})

        location = anchor_component.get_editor_property("relative_location")
        unreal.log(f"{'Added' if existing is None else 'Updated'} {spec['name']} ({spec['tag']}) at "
                   f"{location.x:.0f}, {location.y:.0f}, {location.z:.0f}.")

    unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
    if not unreal.EditorAssetLibrary.save_asset(BLUEPRINT_PATH, only_if_is_dirty=False):
        raise RuntimeError(f"Could not save {BLUEPRINT_PATH} (read-only file or open elsewhere?): the anchors exist only in memory. "
                           "MANUAL: open the Blueprint, Compile, then File > Save All.")
    unreal.log(f"Callout anchors done; {BLUEPRINT_PATH} compiled and saved.")
    unreal.log("MANUAL: check the anchors' places in the Blueprint viewport and move them with the gizmo if needed.")


create_callout_anchors()
