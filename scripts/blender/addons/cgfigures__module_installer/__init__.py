# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTIBILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program. If not, see <http://www.gnu.org/licenses/>.

bl_info = {
    "name" : "CGFigures - Module Installer",
    "author" : "CGFigures", 
    "description" : "Installs python packages into Blender",
    "blender" : (3, 4, 0),
    "version" : (1, 0, 0),
    "location" : "",
    "warning" : "",
    "doc_url": "", 
    "tracker_url": "", 
    "category" : "3D View" 
}


import bpy
import bpy.utils.previews
import sys
import os


addon_keymaps = {}
_icons = None
class SNA_OT_Install_Module_Dcd7D(bpy.types.Operator):
    bl_idname = "sna.install_module_dcd7d"
    bl_label = "install_module"
    bl_description = "Installs the python module entered"
    bl_options = {"REGISTER", "UNDO"}

    @classmethod
    def poll(cls, context):
        if bpy.app.version >= (3, 0, 0) and True:
            cls.poll_message_set('')
        return not False

    def execute(self, context):
        install_package = bpy.context.scene.sna_package
        import subprocess
        import platform

        def isWindows():
            return os.name == 'nt'

        def isMacOS():
            return os.name == 'posix' and platform.system() == "Darwin"

        def isLinux():
            return os.name == 'posix' and platform.system() == "Linux"

        def python_exec():
            import sys
            if isWindows():
                return os.path.join(sys.prefix, 'bin', 'python.exe')
            elif isMacOS():
                try:
                    # 2.92 and older
                    path = bpy.app.binary_path_python
                except AttributeError:
                    # 2.93 and later
                    import sys
                    path = sys.executable
                return os.path.abspath(path)
            elif isLinux():
                return os.path.join(sys.prefix, 'sys.prefix/bin', 'python')
            else:
                print("sorry, still not implemented for ", os.name, " - ", platform.system)

        def installModule(packageName):
            try:
                subprocess.call([python_exe, "import ", packageName])
            except:
                python_exe = python_exec()
               # upgrade pip
                subprocess.call([python_exe, "-m", "ensurepip"])
                subprocess.call([python_exe, "-m", "pip", "install", "--upgrade", "pip"])
               # install required packages
                subprocess.call([python_exe, "-m", "pip", "install", packageName])
        installModule(install_package)
        #credit to luckychris https://github.com/luckychris
        return {"FINISHED"}

    def invoke(self, context, event):
        return self.execute(context)


class SNA_PT_MODULE_INSTALLATION_7CC15(bpy.types.Panel):
    bl_label = 'Module Installation'
    bl_idname = 'SNA_PT_MODULE_INSTALLATION_7CC15'
    bl_space_type = 'PROPERTIES'
    bl_region_type = 'WINDOW'
    bl_context = 'render'
    bl_category = 'PyInstall'
    bl_order = 0
    bl_ui_units_x=0

    @classmethod
    def poll(cls, context):
        return not (False)

    def draw_header(self, context):
        layout = self.layout

    def draw(self, context):
        layout = self.layout
        op = layout.operator('sna.install_module_dcd7d', text='Install Module', icon_value=746, emboss=True, depress=False)
        split_62AB1 = layout.split(factor=0.4444444477558136, align=False)
        split_62AB1.alert = False
        split_62AB1.enabled = True
        split_62AB1.active = True
        split_62AB1.use_property_split = False
        split_62AB1.use_property_decorate = False
        split_62AB1.scale_x = 1.0
        split_62AB1.scale_y = 1.0
        split_62AB1.alignment = 'Expand'.upper()
        if not True: split_62AB1.operator_context = "EXEC_DEFAULT"
        split_62AB1.label(text='Module Name:', icon_value=0)
        split_62AB1.prop(bpy.context.scene, 'sna_package', text='', icon_value=0, emboss=True)


def register():
    global _icons
    _icons = bpy.utils.previews.new()
    bpy.types.Scene.sna_package = bpy.props.StringProperty(name='package', description='', default='', subtype='NONE', maxlen=0)
    bpy.utils.register_class(SNA_OT_Install_Module_Dcd7D)
    bpy.utils.register_class(SNA_PT_MODULE_INSTALLATION_7CC15)


def unregister():
    global _icons
    bpy.utils.previews.remove(_icons)
    wm = bpy.context.window_manager
    kc = wm.keyconfigs.addon
    for km, kmi in addon_keymaps.values():
        km.keymap_items.remove(kmi)
    addon_keymaps.clear()
    del bpy.types.Scene.sna_package
    bpy.utils.unregister_class(SNA_OT_Install_Module_Dcd7D)
    bpy.utils.unregister_class(SNA_PT_MODULE_INSTALLATION_7CC15)
