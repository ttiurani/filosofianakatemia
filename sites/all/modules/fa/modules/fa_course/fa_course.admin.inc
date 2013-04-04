<?php

/**
 * @file
 * Administration functions for Filosofian Akatemia Course module.
 */

/**
 * Menu callback; display course settings form.
 */
function fa_course_settings() {

  // General settings.
  $form['general'] = array(
    '#type' => 'fieldset',
    '#title' => t('General settings'),
    '#collapsible' => TRUE,
    '#collapsed' => FALSE,
    '#group' => 'additional_settings',
  );
  $form['general']['fa_course_materials'] = array(
    '#type' => 'textarea',
    '#title' => t('Options for material promises'),
    '#default_value' => variable_get('fa_course_materials', 'N/A'),
    '#description' => t('Material promise options, one per row.'),
    '#required' => TRUE,
  );

  // Add vertical tabs display if available.
  $form['#pre_render'][] = 'vertical_tabs_form_pre_render';

  return system_settings_form($form);
}
