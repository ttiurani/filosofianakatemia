<?php

/**
 * @file
 * Administration functions for Filosofian Akatemia Academians module.
 */

/**
 * Menu callback; display course settings form.
 */
function fa_academian_settings() {

  // General settings.
  $form['general'] = array(
    '#type' => 'fieldset',
    '#title' => t('General settings'),
    '#collapsible' => TRUE,
    '#collapsed' => FALSE,
    '#group' => 'additional_settings',
  );
  $form['general']['fa_academian_half_max_minutes'] = array(
    '#type' => 'textfield',
    '#title' => t('Maximum number of minutes to count as 0.5 trainer point'),
    '#default_value' => variable_get('fa_academian_half_max_minutes', 60),
    '#description' => t('Maximum number of minutes that is counted as 0.5 trainer points. Above it counts as 1 or more trainer points.'),
    '#required' => TRUE,
  );
  $form['general']['fa_academian_full_max_minutes'] = array(
    '#type' => 'textfield',
    '#title' => t('Maximum number of minutes to count as 1 trainer point'),
    '#default_value' => variable_get('fa_academian_full_max_minutes', 210),
    '#description' => t('Maximum number of minutes that is counted as 1 trainer points. Above it counts as 1.5 or more trainer points.'),
    '#required' => TRUE,
  );
  $form['general']['fa_academian_oneandhalf_max_minutes'] = array(
    '#type' => 'textfield',
    '#title' => t('Maximum number of minutes to count as 1.5 trainer point'),
    '#default_value' => variable_get('fa_academian_oneandhalf_max_minutes', 300),
    '#description' => t('Maximum number of minutes that is counted as 1.5 trainer points. Anything above counts as 2 trainer points.'),
    '#required' => TRUE,
  );

  // Academians options settings.
  $form['academian'] = array(
    '#type' => 'fieldset',
    '#title' => t('Academian settings'),
    '#collapsible' => TRUE,
    '#collapsed' => FALSE,
    '#group' => 'additional_settings',
  );

  $academians = _fa_get_academians();

  foreach($academians as $uid => $email){
    // Academian specific options settings.
    $trainer_points_id = 'fa_academian_trainer_points_' . $uid;
    $form['academian'][$trainer_points_id] = array(
      '#type' => 'textfield',
      '#title' => t('Required trainer points in a half year period for ' . $email),
      '#default_value' => variable_get($trainer_points_id, 0),
      '#required' => TRUE,
    );
  }

  // Add vertical tabs display if available.
  $form['#pre_render'][] = 'vertical_tabs_form_pre_render';

  return system_settings_form($form);
}
