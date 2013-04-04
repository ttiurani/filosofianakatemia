<?php

/**
 * @file
 * Course session editing functions for Filosofian Akatemia Course module.
 */

/**
 * Return a list of all course sessions.
 */
function fa_course_sessions($facid) {
  $header = array(
      array('data' => t('Session Id'), 'field' => 'facsid'),
      t('Session type'),
      array('data' => t('Start time'), 'field' => 'session_start_datetime', 'sort' => 'asc'),
      t('End Time'),
      t('Address'),
      t('Material'),
      t('Assistant email'),
      t('Participants'),
      t('Actions'),
  );

  $session_type = _get_session_types();

  $result = db_select('fa_course_session', 's')
    ->fields('s', array('facid', 'facsid', 'session_type', 'session_start_datetime', 'session_end_datetime', 'address', 'material', 'assistant_email'))
    ->extend('TableSort')
    ->extend('PagerDefault');
  $result->leftJoin('fa_sessions_participants', 'sp', 'sp.facsid = s.facsid');
  $result->addExpression('COUNT(sp.facpid)', 'participants');
  $result->condition('s.facid', $facid);
  $result->distinct();

  $result->groupBy('s.facid');
  $result->groupBy('s.facsid');

  $result->orderByHeader($header);
  $result = $result->execute()->fetchAll();

  $rows = array();
  foreach ($result as $row) {
    $cells = array();
    $cells[] = $row->facsid;
    $cells[] = $session_type[$row->session_type];
    $cells[] = format_date($row->session_start_datetime, 'custom', 'D d.m.Y H:i');
    $cells[] = format_date($row->session_end_datetime, 'custom', 'D d.m.Y H:i');
    $cells[] = $row->address;
    $cells[] = $row->material;
    $cells[] = $row->assistant_email;
    $cells[] = $row->participants;
    $cells[] = (user_access('administer courses'))
                ? l(t('Edit'), "admin/people/courses/sessions/$facid/edit/$row->facsid") . '<br/>' .
                  l(t('Delete'), "admin/people/courses/sessions/$facid/delete/$row->facsid")
                : "";
    $rows[] = $cells;
  }

  $links = '<p>' . l(t("Back to courses"), "admin/people/courses") . '<br/>'
            . l(t("Add new session"), "admin/people/courses/sessions/$facid/edit/new") . '</p>';
  $output = render($links);

  $output .= theme('table', array('header' => $header, 'rows' => $rows));
  if (!$rows) {
    $output .= t('No sessions found.');
  }
  else {
    $output .= theme('pager');
  }

  return $output;
}

function _get_session_types() {
  return array(
      0 => t("Other"),
      1 => t("Lecture"),
      2 => t("Workshop"),
      3 => t("Lecture and Workshop"),
      4 => t("Discussion"),
      5 => t("One on One"),
  );
}

function fa_course_sessions_edit($facid, $facsid){
  $links = '<p>' . l(t("Back to sessions"), "admin/people/courses/sessions/$facid") . '</p>';
  $output = render($links);


  $course_session = _get_course_session($facid, $facsid);
  $edit_form = drupal_get_form('fa_course_session_edit_form', $course_session);
  $output .= render($edit_form);
  return $output;
}

function _get_course_session($facid, $facsid){
  if ($facsid != 'new'){
    $course_session = db_select('fa_course_session', 's')
    ->fields('s')
    ->condition('s.facsid', $facsid)
    ->condition('s.facid', $facid)
    ->execute()->fetchObject();

  }else {
    $course_session = (object) array(
        'facid'  => $facid,
    );
  }
  return $course_session;
}

/**
 * Form for creating/editing a course session.
 */
function fa_course_session_edit_form($form, &$form_state, $course_session) {
  global $user;

  $form['course_session'] = array(
      '#type' => 'fieldset',
      '#title' => t('Edit session'),
  );
  $form['course_session']['id'] = array(
      '#type' => 'textfield',
      '#title' => t('Session Id'),
      '#disabled' => TRUE,
      '#value' => isset($course_session->facsid) ? $course_session->facsid : 'N/A',
  );
  $form['course_session']['facid'] = array(
      '#type' => 'textfield',
      '#title' => t('Course Id'),
      '#disabled' => TRUE,
      '#value' => $course_session->facid,
  );

  $session_type = _get_session_types();

  $form['course_session']['session_type'] = array(
      '#type' => 'select',
      '#title' => t('Session type'),
      '#default_value' => isset($course_session->session_type) ? $course_session->session_type : 1,
      '#options' => _get_session_types(),
  );

  // Start time of course

  $form['course_session']['start_datetime'] = array(
      '#type'           => 'date_popup',
      '#title'          => t('Start date and time for the session'),
      '#date_format'    => DATE_FORMAT,
      '#default_value'  => isset($course_session->session_start_datetime) ? date('Y-m-d H:i', $course_session->session_start_datetime) : date('Y-m-d H:i', REQUEST_TIME),
      '#required'       => TRUE,
  );

  // Length of course
  $form['course_session']['length'] = array(
      '#type'           => 'textfield',
      '#title'          => t('Length of the session in minutes'),
      '#default_value'  => isset($course_session->session_end_datetime) ? ($course_session->session_end_datetime - $course_session->session_start_datetime) / 60: 180,
      '#required'       => TRUE,
  );

  $form['course_session']['address'] = array(
      '#type'           => 'textfield',
      '#title'          => t('Address of session'),
      '#description'          => t('Detailed address for the session, should include all information that is
                                    required for both the participants and the trainer to find their way to the
                                    right place. E.g. room number and contact phone number might be needed.'),
      '#default_value'  => isset($course_session->address) ? $course_session->address : NULL,
  );


  // Material
  $material_choices = preg_split( '/\r\n|\r|\n/', variable_get('fa_course_materials', array('N/A')));
  if (isset($course_session->material) && !in_array($course_session->material, $material_choices)){
    array_push($material_choices, $course_session->material);
  }
  $form['course_session']['material'] = array(
      '#type'           => 'select',
      '#multiple'       => FALSE,
      '#title'          => t('Material promised to the session'),
      '#default_value'  => isset($course_session->material) ? array_search($course_session->material, $material_choices) : 0,
      '#options'        => $material_choices,
  );

  $form['course_session']['summary_emails'] = array(
      '#type'           => 'textfield',
      '#title'          => t('Summary emails, separated with comma'),
      '#default_value'  => isset($course_session->summary_emails) ? $course_session->summary_emails : NULL,
  );

  $form['course_session']['assistant_email'] = array(
      '#type'           => 'textfield',
      '#title'          => t('Assistant email'),
      '#default_value'  => isset($course_session->assistant_email) ? $course_session->assistant_email : NULL,
  );

  $product_name = str_replace('#', '', db_query("SELECT r.name from fa_course fac "
                  . "INNER JOIN role r ON r.rid = fac.product_id "
                  . "WHERE fac.facid = :facid",
                      array(':facid' => $course_session->facid)
                    )->fetchField());

  // Preliminary assignment

  $preliminary_assignment_options = _fa_course_get_assignment_options($product_name, 'preliminary_assignment');
  $form['course_session'] = $form['course_session'] + _get_assignment_form($course_session,
                                                'preliminary_assignment',
                                                'preliminary assignment',
                                                $preliminary_assignment_options,
                                                $product_name);
  // Course assignment

  $course_assignment_options = _fa_course_get_assignment_options($product_name, 'course_assignment');
  $form['course_session'] = $form['course_session'] + _get_assignment_form($course_session,
                                                'course_assignment',
                                                'course assignment',
                                                $course_assignment_options,
                                                $product_name);
  // Feedback

  $feedback_options = _fa_course_get_assignment_options($product_name, 'course_feedback');
  $form['course_session'] = $form['course_session'] + _get_assignment_form($course_session,
                                                'feedback',
                                                'feedback',
                                                $feedback_options,
                                                $product_name);

  $form['course_session']['submit'] = array(
      '#type' => 'submit',
      '#value' => t('Save'),
  );

  $form_state['course_session'] = $course_session;
  return $form;
}

function _get_assignment_form($course_session, $assignment_type, $assignment_type_name, $assignment_options, $product_name) {
  $form = array();

  $assignment_sent_datetime = $assignment_type . '_sent';
  $assignment_sent = isset($course_session->$assignment_sent_datetime);

  $form[$assignment_type] = array(
      '#type' => 'fieldset',
      '#title' => ucfirst($assignment_type_name),
  );

  $assignment_type_node_id = $assignment_type . '_node_id';
  $form[$assignment_type][$assignment_type . '_check'] = array(
      '#type'           => 'checkbox',
      '#title'          => t('This session has a ' . $assignment_type_name),
      '#disabled'       => $assignment_sent || empty($assignment_options),
      '#default_value'  => isset($course_session->$assignment_type_node_id) ? TRUE : FALSE,
  );

  $form[$assignment_type][$assignment_type . '_select'] = array(
      '#type' => 'select',
      '#title' => t(ucfirst($assignment_type_name) . ' form'),
      '#default_value' => isset($course_session->$assignment_type_node_id) ?
                                  $course_session->$assignment_type_node_id :
                                    reset($assignment_options),
      '#options' => $assignment_options,
      '#disabled' => $assignment_sent,
      '#states' => array(
          'visible' => array(
              ':input[name=' . $assignment_type . '_check]' => array('checked' => TRUE),
          ),
      ),
  );

  $template = array();
  $assignment_email_template = $assignment_type . '_email_template';
  if (isset($course_session->$assignment_email_template))
    $template = unserialize($course_session->$assignment_email_template);

  $default_subject_id = 'fa_course_default_' . strtolower($product_name) .'_'. $assignment_type . '_subject_template';
  $form[$assignment_type][$assignment_type . '_subject'] = array(
      '#type'           => 'textfield',
      '#title'          => t(ucfirst($assignment_type_name) . ' email title'),
      '#default_value'  => !empty($template) ? $template['subject'] :
            variable_get($default_subject_id, NULL),
      '#disabled'       => $assignment_sent,
      '#states' => array(
          'visible' => array(
              ':input[name=' . $assignment_type . '_check]' => array('checked' => TRUE),
          ),
      ),
  );

  $default_message_id = 'fa_course_default_' . strtolower($product_name) .'_'. $assignment_type . '_message_template';
  $form[$assignment_type][$assignment_type . '_message'] = array(
      '#type'           => 'textarea',
      '#title'          => t(ucfirst($assignment_type_name) . ' email body'),
      '#default_value'  => !empty($template) ? $template['message'] :
            variable_get($default_message_id, NULL),
      '#disabled'       => $assignment_sent,
      '#states' => array(
          'visible' => array(
              ':input[name=' . $assignment_type . '_check]' => array('checked' => TRUE),
          ),
      ),
  );

  $assignment_datetime = $assignment_type . '_datetime';
  $form[$assignment_type][$assignment_type . '_action'] = array(
      '#type'           => 'radios',
      '#title'          => t(ucfirst($assignment_type_name) . ' action'),
      '#access'         => !$assignment_sent,
      '#options'        => array(SCHEDULE => t('Schedule ' . $assignment_type_name),
                                 SEND_NOW => t('Send ' . $assignment_type_name . ' now')),
      '#default_value'  => isset($course_session->$assignment_datetime) ?
                            SCHEDULE : NULL,
      '#states' => array(
          'visible' => array(
              ':input[name=' . $assignment_type . '_check]' => array('checked' => TRUE),
          ),
      ),
  );

  $default_assignment_datetime = date('Y-m-d H:i', REQUEST_TIME);
  if (isset($course_session->session_start_datetime)){
    $default_assignment_datetime = date('Y-m-d H:i', $course_session->session_start_datetime - (SECONDS_IN_DAY * 14));
  }

  $form[$assignment_type][$assignment_type . '_schedule_datetime'] = array(
      '#type'           => 'date_popup',
      '#title'          => t('Date and time when the ' .  $assignment_type_name . ' should be sent'),
      '#access'         => !$assignment_sent,
      '#date_format'    => DATE_FORMAT,
      '#default_value'  => isset($course_session->$assignment_datetime) ? date('Y-m-d H:i', $course_session->$assignment_datetime) :
                                 $default_assignment_datetime,
      '#states' => array(
          'visible' => array(
              ':input[name=' . $assignment_type . '_check]' => array('checked' => TRUE),
              ':input[name=' . $assignment_type . '_action]' => array('value' => SCHEDULE),
              ),
      ),
  );

  $form[$assignment_type]['sent'] = array(
      '#type' => 'textfield',
      '#title' => t(ucfirst($assignment_type_name) . ' sent time'),
      '#disabled' => TRUE,
      '#access' => $assignment_sent,
      '#value' => isset($course_session->$assignment_sent_datetime) ?
                      format_date($course_session->$assignment_sent_datetime, 'custom', DATE_FORMAT) : NULL,
      '#states' => array(
          'visible' => array(
              ':input[name=' . $assignment_type . '_check]' => array('checked' => TRUE),
          ),
      ),
  );

  $reminder_sent_datetime = $assignment_type . '_reminder_sent';
  $assignment_reminder_datetime = $assignment_type . '_reminder_datetime';
  $form[$assignment_type][$assignment_type . '_reminder_action'] = array(
      '#type'           => 'radios',
      '#title'          => t(ucfirst($assignment_type_name) . ' reminder action'),
      '#access'         => !isset($course_session->$reminder_sent_datetime),
      '#options'        => array(SCHEDULE => t('Schedule ' . $assignment_type_name . ' reminder'),
                                 SEND_NOW => t('Send ' . $assignment_type_name . ' reminder now')),
      '#default_value'  => isset($course_session->$assignment_reminder_datetime) ?
                            SCHEDULE : NULL,
      '#states' => array(
          'visible' => array(
              ':input[name=' . $assignment_type . '_check]' => array('checked' => TRUE),
          ),
      ),
  );

  $default_reminder_datetime = date('Y-m-d H:i', REQUEST_TIME);
  if (isset($course_session->session_start_datetime)){
    $default_reminder_datetime = date('Y-m-d H:i', $course_session->session_start_datetime - (SECONDS_IN_DAY * 2));
  }

  $form[$assignment_type][$assignment_type . '_reminder_schedule_datetime'] = array(
      '#type'           => 'date_popup',
      '#title'          => t('Date and time when a reminder for the ' .  $assignment_type_name . ' should be sent'),
      '#access'         => !isset($course_session->$reminder_sent_datetime),
      '#date_format'    => DATE_FORMAT,
      '#default_value'  => isset($course_session->$assignment_datetime) ? date('Y-m-d H:i', $course_session->$assignment_reminder_datetime) :
                                 $default_reminder_datetime,
      '#states' => array(
          'visible' => array(
              ':input[name=' . $assignment_type . '_check]' => array('checked' => TRUE),
              ':input[name=' . $assignment_type . '_reminder_action]' => array('value' => SCHEDULE),
              ),
      ),
  );

  $form[$assignment_type]['reminder_sent'] = array(
      '#type' => 'textfield',
      '#title' => t(ucfirst($assignment_type_name) . ' reminder sent time'),
      '#disabled' => TRUE,
      '#access' => isset($course_session->$reminder_sent_datetime),
      '#value' => isset($course_session->$assignment_reminder_datetime) ?
                      format_date($course_session->$assignment_reminder_datetime, 'custom', DATE_FORMAT) : NULL,
      '#states' => array(
          'visible' => array(
              ':input[name=' . $assignment_type . '_check]' => array('checked' => TRUE),
          ),
      ),
  );

  return $form;
}

function fa_course_session_edit_form_validate($form, &$form_state) {

  if (!empty($form_state['values']['length'])){
    if (!is_numeric($form_state['values']['length']) ){
      form_set_error('length', t('Length value must be a number'));
      return;
    }else{
      $length = intval($form_state['values']['length']);
      if ($length < 0){
        form_set_error('length', t('Length value must be a positive integer'));
      }else{
        // Get end timestamp by adding the length to the epoch start timestamp
        $form_state['values']['start_datetime_epoch'] = strtotime($form_state['values']['start_datetime']);
        $form_state['values']['end_datetime_epoch'] = $form_state['values']['start_datetime_epoch'] + $length * 60;
      }
    }
  }

  // Check summary emails
  if (empty($form_state['values']['summary_emails']) &&
      ($form_state['values']['preliminary_assignment_check'] ||
      $form_state['values']['course_assignment_check'] ||
      $form_state['values']['feedback_check'])){
    form_set_error('summary_emails', t('Summary emails are mandatory if assignments are sent.'));
  }

  $summary_emails_field = _fa_course_get_emails($form_state['values']['summary_emails']);
  _fa_course_validate_emails($summary_emails_field, 'summary_emails');

  // Check assistant email
  if (!empty($form_state['values']['assistant_email'])){
    $assistant_email_field = _fa_course_get_emails($form_state['values']['assistant_email']);
    _fa_course_validate_one_email($assistant_email_field, 'assistant_email');
  }

  // Preliminary assignment
  _validate_assignment_schedule($form_state, 'preliminary_assignment');

  // Course assignment
  _validate_assignment_schedule($form_state, 'course_assignment');

  // Feedback
  _validate_assignment_schedule($form_state, 'feedback');

}

function _validate_assignment_schedule($form_state, $assignment_type){
  if ($form_state['values'][$assignment_type . '_action'] == SCHEDULE){
    $assignment_datetime = strtotime($form_state['values'][$assignment_type . '_schedule_datetime']);
    if ($assignment_datetime < strtotime("now")){
      form_set_error($assignment_type . '_schedule_datetime', t('Schedule must be in the future.'));
    }
  }

  if ($form_state['values'][$assignment_type . '_reminder_action'] == SEND_NOW){
    $assignment_sent = $assignment_type . '_sent';
    if (!isset($form_state['course_session']->$assignment_sent)){
      form_set_error($assignment_type . '_reminder_action', t('Can not send reminder before assignment!'));
    }
  }elseif($form_state['values'][$assignment_type . '_reminder_action'] == SCHEDULE){
    $reminder_datetime = strtotime($form_state['values'][$assignment_type . '_reminder_schedule_datetime']);
    if ($reminder_datetime < strtotime("now")) {
      form_set_error($assignment_type . 'reminder_schedule_datetime', t('Schedule must be in the future.'));
    }
    if (isset($assignment_datetime) && $reminder_datetime < $assignment_datetime){
      form_set_error($assignment_type . '_reminder_schedule_datetime', t('Reminder schedule must be after the assignment schedule.'));
    }
  }
}

function fa_course_session_edit_form_submit($form, &$form_state) {
  $course_session = $form_state['course_session'];

  $course_session->session_type = $form_state['values']['session_type'];
  $course_session->session_start_datetime = $form_state['values']['start_datetime_epoch'];
  $course_session->session_end_datetime = $form_state['values']['end_datetime_epoch'];
  $course_session->address = $form_state['values']['address'];
  $course_session->material = $form['course_session']['material']['#options'][$form_state['values']['material']];
  $course_session->summary_emails = $form_state['values']['summary_emails'];
  $course_session->assistant_email = $form_state['values']['assistant_email'];

  _store_assignment($course_session, $form_state, 'preliminary_assignment');
  _store_assignment($course_session, $form_state, 'course_assignment');
  _store_assignment($course_session, $form_state, 'feedback');

  if (isset($course_session->facsid))
    drupal_write_record('fa_course_session', $course_session, 'facsid');
  else
    drupal_write_record('fa_course_session', $course_session);
  drupal_set_message(t('Course session @facsid saved.', array('@facsid' => $course_session->facsid)));
  $form_state['redirect'] = "admin/people/courses/sessions/" . $course_session->facid;
}

function _store_assignment($course_session, $form_state, $assignment_type){
  // Save assignments only if they haven't been sent already
  $assignment_sent = $assignment_type . '_sent';
  if ($form_state['values'][$assignment_type . '_check'] && !isset($course_session->$assignment_sent)){
    $assignment_node_id = $assignment_type . '_node_id';
    $course_session->$assignment_node_id = $form_state['values'][$assignment_type . '_select'];
    $template = array();
    $template['subject'] = $form_state['values'][$assignment_type . '_subject'];
    $template['message'] = $form_state['values'][$assignment_type . '_message'];
    $assignment_email_template = $assignment_type . '_email_template';
    $course_session->$assignment_email_template = serialize($template);
    if (isset($form_state['values'][$assignment_type . '_action']))
      if ($form_state['values'][$assignment_type . '_action'] == SCHEDULE){
        $assignment_datetime = $assignment_type . '_datetime';
        $course_session->$assignment_datetime = strtotime($form_state['values'][$assignment_type . '_schedule_datetime']);
      }else{
        dpm("Immediate send not implemented!");
      }
  }
}


function fa_course_sessions_delete($facid, $facsid){
  $links = '<p>' . l(t("Back to sessions"), "admin/people/courses/sessions/$facid") . '</p>';
  $output = render($links);
  $session = _get_course_session($facid, $facsid);
  $delete_form = drupal_get_form('fa_course_sessions_delete_form', $session);
  $output .= render($delete_form);
  return $output;
}

function fa_course_sessions_delete_form($form, &$form_state, $session) {
  $form['session'] = array(
    '#type' => 'value',
    '#value' => $session,
  );

  return confirm_form(
    $form,
    t('Are you sure you want to delete the session %facsid?', array('%facsid' => $session->facsid)),
    "admin/people/courses/sessions/" . $session->facid,
    t('This action cannot be undone.'),
    t('Delete'),
    t('Cancel')
  );
}

function fa_course_sessions_delete_form_submit($form, &$form_state) {
  $session = $form_state['values']['session'];

  db_delete('fa_course_session')
    ->condition('facsid', $session->facsid)
    ->execute();

  drupal_set_message(t('Session with id %facsid has been deleted.', array('%facsid' => $session->facsid)));
  $form_state['redirect'] = "admin/people/courses/sessions/" . $session->facid;
}
