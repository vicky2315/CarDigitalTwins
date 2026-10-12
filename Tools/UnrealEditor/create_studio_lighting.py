"""Creates or updates the studio lights and exposure volume in the open level (build page step A5).

Run inside the Unreal editor (Python Editor Script Plugin) with L_TwinStudio open, from the Output Log with the Cmd box:
    py "<project>/Tools/UnrealEditor/create_studio_lighting.py"

Actors are found by label: missing ones are spawned, existing ones get the values below again, so changing a number here and
re-running is the way to tweak the look. Anything you moved or edited by hand on these five actors is overwritten.
Saves the level at the end.

KeyLight / RimLight / FillLight are Rect Lights (softboxes, their reflections on the paint give the studio look), all Movable
so no light build is needed. StudioPostProcess is an unbound volume with manual exposure: the dark scene isn't brightened.
"""

import unreal

LEVEL_NAME = "L_TwinStudio"

# Location (cm) and rotation in degrees. Rotation is (roll = X, pitch = Y, yaw = Z).
RECT_LIGHTS = [
    {"label": "KeyLight",  "location": (450, -350, 450), "rotation": (0, -40, 142),  "candelas": 150,
     "width": 400, "height": 150, "radius": 2000, "color_srgb_hex": "FFF4E8"},
    {"label": "RimLight",  "location": (-500, 300, 400), "rotation": (0, -33, -31),  "candelas": 100,
     "width": 300, "height": 100, "radius": 2000, "color_srgb_hex": "DCEBFF"},
    {"label": "FillLight", "location": (350, 450, 150), "rotation": (0, -15, -128), "candelas": 30,
     "width": 300, "height": 150, "radius": 2000, "color_srgb_hex": None},  # None = the default white
]

POST_PROCESS_LABEL = "StudioPostProcess"
EXPOSURE_COMPENSATION = 0.0  # raise to 1 if the scene is too dark, lower if too bright
VIGNETTE_INTENSITY = 0.6

actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)


def find_actor_by_label(label, actor_class):
    for actor in actor_subsystem.get_all_level_actors():
        if actor.get_actor_label() == label and isinstance(actor, actor_class):
            return actor
    return None


def find_or_spawn(label, actor_class):
    actor = find_actor_by_label(label, actor_class)
    if actor is not None:
        return actor, False
    actor = actor_subsystem.spawn_actor_from_class(actor_class, unreal.Vector(0, 0, 0))
    if actor is None:
        raise RuntimeError(f"Could not spawn {label}.")
    actor.set_actor_label(label)
    return actor, True


def srgb_color_from_hex(srgb_hex):
    """The light's Light Color is an 8-bit sRGB colour, the same as the picker's Hex sRGB field."""
    red, green, blue = (int(srgb_hex[index:index + 2], 16) for index in (0, 2, 4))
    return unreal.Color(red, green, blue, 255)


def set_properties(target, properties):
    """set_editor_property per entry; names the property that failed instead of a bare exception."""
    for name, value in properties.items():
        try:
            target.set_editor_property(name, value)
        except Exception as error:
            raise RuntimeError(f"Could not set {name} on {target.get_name()}: {error}") from error


def apply_rect_light(spec):
    light_actor, was_created = find_or_spawn(spec["label"], unreal.RectLight)
    light_actor.set_actor_location(unreal.Vector(*spec["location"]), False, True)
    roll, pitch, yaw = spec["rotation"]
    light_actor.set_actor_rotation(unreal.Rotator(roll=roll, pitch=pitch, yaw=yaw), False)

    light_component = light_actor.get_component_by_class(unreal.RectLightComponent)
    properties = {
        "mobility": unreal.ComponentMobility.MOVABLE,
        "intensity_units": unreal.LightUnits.CANDELAS,
        "intensity": float(spec["candelas"]),
        "source_width": float(spec["width"]),
        "source_height": float(spec["height"]),
        "attenuation_radius": float(spec["radius"]),
    }
    if spec["color_srgb_hex"]:
        properties["light_color"] = srgb_color_from_hex(spec["color_srgb_hex"])
    set_properties(light_component, properties)
    unreal.log(f"{'Created' if was_created else 'Updated'} {spec['label']}.")


def apply_post_process_volume():
    volume, was_created = find_or_spawn(POST_PROCESS_LABEL, unreal.PostProcessVolume)
    volume.set_editor_property("unbound", True)

    # Every setting needs its override flag, otherwise the volume leaves it at the project default.
    settings = volume.get_editor_property("settings")
    set_properties(settings, {
        "override_auto_exposure_method": True,
        "auto_exposure_method": unreal.AutoExposureMethod.AEM_MANUAL,
        "override_auto_exposure_apply_physical_camera_exposure": True,
        "auto_exposure_apply_physical_camera_exposure": False,
        "override_auto_exposure_bias": True,
        "auto_exposure_bias": EXPOSURE_COMPENSATION,
        "override_vignette_intensity": True,
        "vignette_intensity": VIGNETTE_INTENSITY,
    })
    volume.set_editor_property("settings", settings)  # structs come back as copies: write it back
    unreal.log(f"{'Created' if was_created else 'Updated'} {POST_PROCESS_LABEL}.")


def create_studio_lighting():
    level_name = unreal.EditorLevelLibrary.get_editor_world().get_name()
    if level_name != LEVEL_NAME:
        raise RuntimeError(f"The open level is {level_name}; open {LEVEL_NAME} first.")

    for spec in RECT_LIGHTS:
        apply_rect_light(spec)
    apply_post_process_volume()

    unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
    unreal.log(f"Studio lighting done and {LEVEL_NAME} saved. Judge the look in the viewport, then tune the numbers at the top and re-run.")


create_studio_lighting()
