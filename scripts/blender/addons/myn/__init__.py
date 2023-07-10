bl_info = {
    "name": "myn",
    "author": "miyehn",
    "blender": (3, 2, 0),
    "category": "Personal",
}

import bpy

from .properties import *
from .export_glb import *
from .quickops import *
from .panel import *

#============================= DEV =================================


#===================================================================

addon_keymaps = []

def init_keymaps():
    kc = bpy.context.window_manager.keyconfigs.addon
    km = kc.keymaps.new(name="3D View", space_type="VIEW_3D")
    kmi = [
        km.keymap_items.new("myn.export_glb", 'E', 'PRESS', ctrl=True, shift=True)
    ]
    return km, kmi

classes = (
    MynProperties,
    MYN_OT_export_glb,
    MYN_OT_origin_to_selected,
    MYN_OT_triangulate_selected,
    MYN_OT_open_scripts_dir,
    MYN_PT_tools_panel
)

def register():
    for cls in classes: bpy.utils.register_class(cls)

    # global props
    bpy.types.Scene.myn_props = bpy.props.PointerProperty(type=MynProperties)

    # keymap
    if (not bpy.app.background):
        km, kmi = init_keymaps()
        for k in kmi:
            k.active = True
            addon_keymaps.append((km, k))
    print("loaded myn. num keymaps: " + str(len(addon_keymaps)))


def unregister():
    for cls in classes: bpy.utils.unregister_class(cls)

    # global props
    del bpy.types.Scene.myn_props

    # keymap
    for km, kmi in addon_keymaps:
        km.keymap_items.remove(kmi)
    addon_keymaps.clear()
    print("unloaded myn.")
