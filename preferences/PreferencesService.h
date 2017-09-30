//
// Created by martin on 15/04/17.
//

#ifndef CALENHAD_PREFERENCESSERVICE_H
#define CALENHAD_PREFERENCESSERVICE_H


#include <QtCore/QSettings>
#include <QColor>

namespace calenhad {
    namespace preferences {
        class PreferencesService {
        public:
            virtual void saveSettings () = 0;

            virtual void loadSettings () = 0;

            virtual QSettings* settings () = 0;

            // Colours and styles

            QString calenhad_stylesheet;
            QColor calenhad_module_brush_color_selected;
            QColor calenhad_module_brush_color_normal;
            QColor calenhad_nodegroup_brush_color_selected;
            QColor calenhad_nodegroup_brush_color_normal;
            QColor calenhad_handle_brush_color_bg;
            QColor calenhad_port_in_border_color;
            QColor calenhad_port_in_fill_color;
            QColor calenhad_port_in_fill_color_drop;
            QColor calenhad_port_in_border_color_drop;
            QColor calenhad_port_in_fill_color_connected;
            QColor calenhad_port_in_border_color_connected;
            QColor calenhad_port_out_border_color;
            QColor calenhad_port_out_fill_color;
            unsigned calenhad_port_border_weight;
            QColor calenhad_port_text_color;
            QColor calenhad_connector_normal_color;
            unsigned calenhad_connector_normal_weight;
            QColor calenhad_connector_selected_color;
            unsigned calenhad_connector_selected_weight;
            QColor calenhad_connector_drawing_color;
            unsigned calenhad_connector_drawing_weight;
            QColor calenhad_connector_inactive_color;
            unsigned calenhad_connector_inactive_weight;
            QColor calenhad_module_text_color_selected;
            QColor calenhad_module_text_color_normal;
            QColor calenhad_nodegroup_text_color_selected;
            QColor calenhad_nodegroup_text_color_normal;
            QColor calenhad_module_text_color_error;
            QColor calenhad_variables_color_normal;
            QColor calenhad_variables_color_error;

            // Configuration

            QString calenhad_legends_filename;
            int calenhad_variables_name_maxlength;
            int calenhad_port_name_minLength;
            int calenhad_port_name_maxLength;
            int calenhad_node_name_minLength;
            int calenhad_node_name_maxLength;
            QString calenhad_node_name_validChars;


            // Geometry

            unsigned calenhad_port_radius;
            unsigned calenhad_port_margin;
            unsigned calenhad_module_duplicate_offset_x;
            unsigned calenhad_module_duplicate_offset_y;
            unsigned calenhad_altitudemap_deletemargin;
            unsigned calenhad_altitudemap_buffersize;
            unsigned calenhad_colormap_buffersize;
            int calenhad_toolpalette_icon_size;
            int calenhad_toolpalette_icon_margin;
            int calenhad_toolpalette_icon_shadow;
            int calenhad_toolpalette_arrow_size;
            int calenhad_handle_module_width;
            int calenhad_handle_module_height;
            int calenhad_handle_module_margin;
            int calenhad_notifications_width;
            int calenhad_notifications_margin;

            // Modules

            QString calenhad_module_perlin;
            QString calenhad_module_cylinders;
            QString calenhad_module_spheres;
            QString calenhad_module_exponent;
            QString calenhad_module_translate;
            QString calenhad_module_rotate;
            QString calenhad_module_clamp;
            QString calenhad_module_constant;
            QString calenhad_module_add;
            QString calenhad_module_abs;
            QString calenhad_module_blend;
            QString calenhad_module_invert;
            QString calenhad_module_max;
            QString calenhad_module_min;
            QString calenhad_module_multiply;
            QString calenhad_module_power;
            QString calenhad_module_displace;
            QString calenhad_module_diff;
            QString calenhad_module_billow;
            QString calenhad_module_ridgedmulti;
            QString calenhad_module_scalebias;
            QString calenhad_module_select;
            QString calenhad_module_turbulence;
            QString calenhad_module_voronoi;
            QString calenhad_module_scalepoint;
            QString calenhad_module_icospheremap;
            QString calenhad_module_altitudemap;
            QString calenhad_nodegroup;
            QColor calenhad_toolpalette_icon_color_normal;
            QColor calenhad_toolpalette_icon_color_mouseover;
            QColor calenhad_toolpalette_icon_color_shadow;


        };
    }
}


#endif //CALENHAD_PREFERENCESSERVICE_H
