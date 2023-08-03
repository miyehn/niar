import bpy
import bmesh

class MYN_OT_export_glb(bpy.types.Operator):
    bl_idname = "myn.export_glb"
    bl_label = "Triangulate all and export glb"
    bl_options = {'REGISTER', 'UNDO'}
    
    @staticmethod
    def postprocess_materials(obj):
        obj.report({'INFO'}, "hi")

        # loop through materials
        for i in range(0, len(bpy.data.materials)):
            mat = bpy.data.materials[i]
            # obj.report({'INFO'}, mat.name)
            if mat.node_tree and len(mat.node_tree.nodes['Material Output'].inputs['Volume'].links) > 0:
                volume_node = mat.node_tree.nodes['Material Output'].inputs['Volume'].links[0].from_node
                obj.report({'INFO'}, "--volume: " + volume_node.name)
                if volume_node.name == 'Principled Volume':
                    # found a valid volume node, append its core properties to the material
                    mat['_is_volume'] = True
                    mat['_volume_color'] = volume_node.inputs['Color'].default_value
                    mat['_volume_density'] = volume_node.inputs['Density'].default_value
        return
    
    def execute(self, context):
        
        if (not bpy.data.is_saved):
            self.report({'INFO'}, "export failed: file is not saved to disk!")
            return {'FINISHED'}

        bpy.ops.object.mode_set(mode='OBJECT')
        
        # all mesh data (keep a copy of bmesh original)
        meshes = list()
        bmeshes_original = list()
        
        # add to lists
        for i in range(0, len(bpy.context.scene.objects)):
            obj = bpy.context.scene.objects[i]
            if (obj.type == 'MESH'):
                # meshes
                meshes.append(obj.data)
                # original bmeshes
                bm_o = bmesh.new()
                bm_o.from_mesh(obj.data)
                bmeshes_original.append(bm_o)
                # triangulated bmeshes
                bm_t = bmesh.new()
                bm_t.from_mesh(obj.data)
                bmesh.ops.triangulate(bm_t, faces=bm_t.faces[:])
                bm_t.to_mesh(obj.data)
                bm_t.free()
                obj.data.update()
        
        # export
        # see: https://docs.blender.org/api/current/bpy.ops.export_scene.html
        path=bpy.path.abspath("//" + bpy.context.scene.myn_props.export_path)
        bpy.ops.export_scene.gltf(
            filepath=path,
            check_existing=False,
            export_format='GLB',
            export_image_format='AUTO',
            export_texcoords=True,
            export_normals=True,
            export_tangents=True,
            export_materials='EXPORT',
            export_colors=False,
            export_cameras=True,
            #export_selected=False,
            use_selection=False,
            use_visible=True,
            use_renderable=False,
            use_active_collection=False,
            export_extras=True,
            export_yup=False,
            export_apply=False,
            export_animations=False,
            export_lights=True,
            # TODO: take this out when work on niar physical light units in the future
            #export_import_convert_lighting_mode='RAW'
            )
        
        # restore originals
        for i in range(0, len(meshes)):
            bm_o = bmeshes_original[i]
            bm_o.to_mesh(meshes[i])
            bm_o.free()
            
        MYN_OT_export_glb.postprocess_materials(self);
        
        # report and exit
        self.report({'INFO'}, path)

        # potentially bring niar to front:
        try:
            import pygetwindow as gw
            windows = gw.getWindowsWithTitle('niar - main window')
            if (len(windows) > 0):
                windows[0].activate()
        except ModuleNotFoundError:
            pass
        
        return {'FINISHED'}

