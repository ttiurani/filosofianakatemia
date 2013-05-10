<?php

/**
 * @file
 * Rules module hooks for Filosofian Akatemia Course module.
 */

/**
 * Implements hook_rules_action_info().
 */
function fa_course_rules_action_info() {
  return array(
    'fa_course_action_send_assignment' => array(
      'label' => t('Send preliminary assignment'),
      'group' => t('Filosofian Akatemia'),
      'parameter' => array(
        'facsid' => array(
          'type' => 'value',
          'label' => t('Course session id'),
        ),
        'assignment_type' => array(
          'type' => 'string',
          'label' => t('Assignment type to send'),
        ),
      ),
    ),
  );
}

function fa_course_action_send_assignment($facsid, $assignment_type) {
  drupal_set_message("fa_course_action_send_assignment " . $facsid . $assignment_type);
}
