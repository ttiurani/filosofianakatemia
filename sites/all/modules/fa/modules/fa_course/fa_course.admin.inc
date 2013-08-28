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
  $form['general']['fa_course_default_from_email'] = array(
    '#type' => 'textfield',
    '#title' => t('Default from email'),
    '#default_value' => variable_get('fa_course_default_from_email', 'webmaster@filosofianakatemia.fi'),
    '#description' => t('The from email address for summary emails.'),
    '#required' => TRUE,
  );
  $form['general']['fa_course_default_to_email'] = array(
    '#type' => 'textfield',
    '#title' => t('Default to email'),
    '#default_value' => variable_get('fa_course_default_to_email', 'info@filosofianakatemia.fi'),
    '#description' => t('The default email address when multiple emails can be sent with one message using the bcc field.'),
    '#required' => TRUE,
  );
  $form['general']['fa_course_default_summary_emails'] = array(
    '#type' => 'textfield',
    '#title' => t('Default summary emails'),
    '#default_value' => variable_get('fa_course_default_summary_emails', 'info@filosofianakatemia.fi'),
    '#description' => t('The default summary email address, in addition to the lead trainer email.'),
    '#required' => TRUE,
  );
  $form['general']['fa_course_billing_emails'] = array(
    '#type' => 'textfield',
    '#title' => t('Emails where new course notification and billing information after first session will be sent'),
    '#default_value' => variable_get('fa_course_billing_emails', NULL),
    '#description' => t('The default new course and billing addresses, separated with a comma. Note: lead trainer and sales will be CC:d.'),
    '#required' => TRUE,
  );
  $form['general']['fa_course_capsulecrm_api_authentication_token'] = array(
    '#type' => 'textfield',
    '#title' => t('CapsuleCRM API authentication token'),
    '#default_value' => variable_get('fa_course_capsulecrm_api_authentication_token', NULL),
    '#required' => TRUE,
  );

  $form['general']['fa_course_products'] = array(
    '#type' => 'textarea',
    '#title' => t('Options for material promises'),
    '#default_value' => variable_get('fa_course_products', 'Kehityspolku'),
    '#description' => t('Course products, one per row.'),
    '#required' => TRUE,
  );

  $form['general']['fa_course_materials'] = array(
    '#type' => 'textarea',
    '#title' => t('Options for material promises'),
    '#default_value' => variable_get('fa_course_materials', 'N/A'),
    '#description' => t('Material promise options, one per row.'),
    '#required' => TRUE,
  );

  // Module options settings.
  $form['module'] = array(
    '#type' => 'fieldset',
    '#title' => t('Module settings'),
    '#collapsible' => TRUE,
    '#collapsed' => FALSE,
    '#group' => 'additional_settings',
  );

  $course_roles = _fa_course_get_course_roles();

  foreach($course_roles['modules'] as $module_name){
    // Module specific options settings.
    $module = strtolower($module_name);
    $form['module'][$module] = array(
      '#type' => 'fieldset',
      '#title' => t($module_name . ' settings'),
      '#collapsible' => TRUE,
      '#collapsed' => TRUE,
      '#group' => 'additional_settings',
    );
    $form['module'][$module] = $form['module'][$module]
          + fa_course_get_default_assignment_form(unserialize(PRELIMINARY_ASSIGNMENT),
                                         $module);
    $form['module'][$module] = $form['module'][$module]
          + fa_course_get_default_assignment_form(unserialize(COURSE_ASSIGNMENT),
                                         $module);
    $form['module'][$module] = $form['module'][$module]
          + fa_course_get_default_assignment_form(unserialize(FOLLOWUP),
                                         $module);
    $form['module'][$module] = $form['module'][$module]
          + fa_course_get_default_assignment_form(unserialize(FEEDBACK),
                                         $module);
  }

  // Add vertical tabs display if available.
  $form['#pre_render'][] = 'vertical_tabs_form_pre_render';

  return system_settings_form($form);
}

function fa_course_get_default_assignment_form($assignment, $module) {
  $form = array();

  $subject_id = 'fa_course_default_' . $module .'_'. $assignment['type'] . '_subject_template';
  $form[$subject_id] = array(
    '#type' => 'textfield',
    '#title' => t('Default subject for ' . $assignment['name']),
    '#default_value' => variable_get($subject_id, NULL),
    '#required' => FALSE,
  );
  $message_id = 'fa_course_default_' . $module .'_'. $assignment['type'] . '_message_template';
  $form[$message_id] = array(
    '#type' => 'textarea',
    '#title' => t('Default message for ' . $assignment['name']),
    '#default_value' => variable_get($message_id, NULL),
    '#required' => FALSE,
  );

  return $form;
}
