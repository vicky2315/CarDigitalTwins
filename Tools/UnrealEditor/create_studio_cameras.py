"""Creates or updates the four dashboard view cameras in the open level (build page step A8).

Run inside the Unreal editor (Python Editor Script Plugin) with L_TwinStudio open, from the Output Log with the Cmd box:
    py "<project>/Tools/UnrealEditor/create_studio_cameras.py"

Cameras are found by label: missing ones are spawned, existing ones get the values below again. Each camera gets its actor tag
(ATwinOpsPlayerController finds the camera by it), Field of View 40 and Constrain Aspect Ratio off (otherwise black bars).
Needs no car: the transforms are the page's calculated starting points, framing by eye (pilot the camera, step F3) comes later.
Saves the level at the end.
"""

import unreal

LEVEL_NAME = "L_TwinStudio"
FIELD_OF_VIEW = 40.0

# Location (cm) and rotation in degrees. Rotation is (roll = X, pitch = Y, yaw = Z).
VIEW_CAMERAS = [
    {"label": "CAM_Overview",   "tag": "TwinView.Overview",   "location": (560, -560, 300),  "rotation": (0, -16, 139)},
    {"label": "CAM_Powertrain", "tag": "TwinView.Powertrain", "location": (380, -250, 230),  "rotation": (0, -20, 129)},
    {"label": "CAM_Tyres",      "tag": "TwinView.Tyres",      "location": (0, 90, 1150),     "rotation": (0, -89, 0)},
    {"label": "CAM_Body",       "tag": "TwinView.Body",       "location": (-20, -800, 130),  "rotation": (0, -4, 96)},
]

actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)


def find_or_spawn_camera(label):
    for actor in actor_subsystem.get_all_level_actors():
        if actor.get_actor_label() == label and isinstance(actor, unreal.CameraActor):
            return actor, False
    camera_actor = actor_subsystem.spawn_actor_from_class(unreal.CameraActor, unreal.Vector(0, 0, 0))
    if camera_actor is None:
        raise RuntimeError(f"Could not spawn {label}.")
    camera_actor.set_actor_label(label)
    return camera_actor, True


def set_properties(target, properties):
    """set_editor_property per entry; names the property that failed instead of a bare exception."""
    for name, value in properties.items():
        try:
            target.set_editor_property(name, value)
        except Exception as error:
            raise RuntimeError(f"Could not set {name} on {target.get_name()}: {error}") from error


def apply_view_camera(spec):
    camera_actor, was_created = find_or_spawn_camera(spec["label"])
    camera_actor.set_actor_location(unreal.Vector(*spec["location"]), False, True)
    roll, pitch, yaw = spec["rotation"]
    camera_actor.set_actor_rotation(unreal.Rotator(roll=roll, pitch=pitch, yaw=yaw), False)

    # The tag list is replaced, not appended to, so a re-run can't leave a stale tag behind.
    set_properties(camera_actor, {"tags": [unreal.Name(spec["tag"])]})
    set_properties(camera_actor.get_component_by_class(unreal.CameraComponent), {
        "field_of_view": FIELD_OF_VIEW,
        "constrain_aspect_ratio": False,
    })
    unreal.log(f"{'Created' if was_created else 'Updated'} {spec['label']} ({spec['tag']}).")


def create_studio_cameras():
    level_name = unreal.EditorLevelLibrary.get_editor_world().get_name()
    if level_name != LEVEL_NAME:
        raise RuntimeError(f"The open level is {level_name}; open {LEVEL_NAME} first.")

    for spec in VIEW_CAMERAS:
        apply_view_camera(spec)

    unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
    unreal.log(f"View cameras done and {LEVEL_NAME} saved. Frame them by eye once the car is in (step F3).")


create_studio_cameras()
