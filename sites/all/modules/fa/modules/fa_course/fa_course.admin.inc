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

  // Products options settings.
  $form['product'] = array(
    '#type' => 'fieldset',
    '#title' => t('Product settings'),
    '#collapsible' => TRUE,
    '#collapsed' => FALSE,
    '#group' => 'additional_settings',
  );

  $course_roles = _fa_course_get_course_roles();

  foreach($course_roles['products'] as $product_name){
    // Product specific options settings.
    $product = strtolower($product_name);
    $form['product'][$product] = array(
      '#type' => 'fieldset',
      '#title' => t($product_name . ' settings'),
      '#collapsible' => TRUE,
      '#collapsed' => TRUE,
      '#group' => 'additional_settings',
    );
    $form['product'][$product] = $form['product'][$product]
          + _get_default_assignment_form('preliminary_assignment',
                                         t('preliminary assignment'),
                                         $product);
    $form['product'][$product] = $form['product'][$product]
          + _get_default_assignment_form('course_assignment',
                                         t('course assignment'),
                                         $product);
    $form['product'][$product] = $form['product'][$product]
          + _get_default_assignment_form('feedback',
                                         t('feedback'),
                                         $product);
  }

  // Add vertical tabs display if available.
  $form['#pre_render'][] = 'vertical_tabs_form_pre_render';

  return system_settings_form($form);
}

function _get_default_assignment_form($assignment_type, $assignment_type_name, $product) {
  $form = array();

  $subject_id = 'fa_course_default_' . $product .'_'. $assignment_type . '_subject_template';
  $form[$subject_id] = array(
    '#type' => 'textfield',
    '#title' => t('Default subject for ' . $assignment_type_name),
    '#default_value' => variable_get($subject_id, NULL),
    '#required' => FALSE,
  );
  $message_id = 'fa_course_default_' . $product .'_'. $assignment_type . '_message_template';
  $form[$message_id] = array(
    '#type' => 'textarea',
    '#title' => t('Default message for ' . $assignment_type_name),
    '#default_value' => variable_get($message_id, NULL),
    '#required' => FALSE,
  );

  return $form;
}
