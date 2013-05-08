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
      t('Actions'),
  );

  $session_type = _fa_course_get_session_types();

  $result = db_select('fa_course_session', 's')
    ->fields('s', array('facid', 'facsid', 'session_type', 'session_start_datetime', 'session_end_datetime', 'address', 'material', 'assistant_email'))
    ->extend('TableSort')
    ->extend('PagerDefault');
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
    $cells[] = (user_access('administer courses'))
                ? l(t('Edit'), "admin/people/courses/sessions/$facid/edit/$row->facsid") . '<br/>' .
                  l(t('Delete'), "admin/people/courses/sessions/$facid/delete/$row->facsid")
                : "";
    $rows[] = $cells;
  }

  $links = '<p><strong>' . _fa_course_get_course_name($facid) . '</strong><br/>'
            . l(t("Back"), "admin/people/courses/view/$facid") . '<br/>'
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

function _fa_course_get_session_types() {
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
  $links = '<p><strong>' . _fa_course_get_course_name($facid) . '</strong><br/>'
           . l(t("Back to sessions"), "admin/people/courses/sessions/$facid") . '</p>';
  $output = render($links);


  $course_session = _fa_course_get_course_session($facid, $facsid);
  $edit_form = drupal_get_form('fa_course_session_edit_form', $course_session);
  $output .= render($edit_form);
  return $output;
}

function _fa_course_get_course_session($facid, $facsid){
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
      '#size'           => 10,
      '#maxlength'      => 10,
  );
  $form['course_session']['facid'] = array(
      '#type' => 'textfield',
      '#title' => t('Course Id'),
      '#disabled' => TRUE,
      '#value' => $course_session->facid,
      '#size'           => 10,
      '#maxlength'      => 10,
  );

  $session_type = _fa_course_get_session_types();

  $form['course_session']['session_type'] = array(
      '#type' => 'select',
      '#title' => t('Session type'),
      '#default_value' => isset($course_session->session_type) ? $course_session->session_type : 1,
      '#options' => _fa_course_get_session_types(),
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
      '#size'           => 4,
      '#maxlength'      => 4,

  );

  $form['course_session']['address'] = array(
      '#type'           => 'textfield',
      '#title'          => t('Address of session'),
      '#description'          => t('Detailed address for the session, should include all information that is
                                    required for both the participants and the trainer to find their way to the
                                    right place. E.g. room number and contact phone number might be needed.'),
      '#default_value'  => isset($course_session->address) ? $course_session->address : NULL,
      '#size'           => 100,
      '#maxlength'      => 512,

  );

  // Material
  $material_choices = preg_split( '/\r\n|\r|\n/', variable_get('fa_course_materials', 'N/A'));
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

  $default_summary_emails = NULL;
  $lead_trainer_info = _fa_course_get_lead_trainer_info($course_session);
  if ($lead_trainer_info != FALSE){
    $default_summary_emails = variable_get('fa_course_default_summary_emails', NULL);
    if ($default_summary_emails != NULL) $default_summary_emails .= ', ';
    $default_summary_emails .= _fa_course_get_lead_trainer_info($course_session)->mail;
  }

  $form['course_session']['summary_emails'] = array(
      '#type'           => 'textfield',
      '#title'          => t('Summary emails, separated with comma'),
      '#default_value'  => isset($course_session->summary_emails) ? $course_session->summary_emails :
                                $default_summary_emails,
      '#size'           => 100,
      '#maxlength'      => 512,
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
  $form['course_session'] = $form['course_session'] + _fa_course_get_assignment_form($course_session,
                                                unserialize(PRELIMINARY_ASSIGNMENT),
                                                $preliminary_assignment_options,
                                                $product_name);
  // Course assignment

  $course_assignment_options = _fa_course_get_assignment_options($product_name, 'course_assignment');
  $form['course_session'] = $form['course_session'] + _fa_course_get_assignment_form($course_session,
                                                unserialize(COURSE_ASSIGNMENT),
                                                $course_assignment_options,
                                                $product_name);
  // Followup
  $followup = unserialize(FOLLOWUP);
  $form['course_session'][$followup['type']] = array(
      '#type' => 'fieldset',
      '#title' => ucfirst($followup['name']),
  );

  $form['course_session'][$followup['type']][$followup['type'] . '_check'] = array(
      '#type'           => 'checkbox',
      '#title'          => t('This session has a ' . $followup['name']),
      '#default_value'  => isset($course_session->followup_email_template),
  );

  $form['course_session'][$followup['type']] = $form['course_session'][$followup['type']]
            + _fa_course_get_email_template($course_session, $followup,
              isset($course_session->followup_sent),
              $product_name);

  $form['course_session'][$followup['type']][$followup['type'] . '_action'] = array(
      '#type'           => 'checkbox',
      '#title'          => t('Send ' . $followup['name']),
      '#default_value'  => FALSE,
      '#states' => array(
          'visible' => array(
              ':input[name=' . $followup['type'] . '_check]' => array('checked' => TRUE),
          ),
      ),
  );

  $form['course_session'][$followup['type']][$followup['type'] . '_send_now'] = array(
      '#type' => 'submit',
      '#value' => t('Send now'),
      '#submit' => array('fa_course_session_send_message'),
      '#states' => array(
          'visible' => array(
              ':input[name=' . $followup['type'] . '_check]' => array('checked' => TRUE),
              ':input[name=' . $followup['type'] . '_action]' => array('checked' => TRUE),
              ),
      ),
  );

  // Feedback

  $feedback_options = _fa_course_get_assignment_options($product_name, 'course_feedback');
  $form['course_session'] = $form['course_session'] + _fa_course_get_assignment_form($course_session,
                                                unserialize(FEEDBACK),
                                                $feedback_options,
                                                $product_name);

  $form['course_session']['submit'] = array(
      '#type' => 'submit',
      '#value' => t('Save'),
  );

  $form_state['course_session'] = $course_session;


  if(!isset($form_state['storage']['confirm'])){
    return $form;
  }else{
    $form['course_session']['#type'] = 'hidden';
    $assignment_type = $form_state['send_info']['assignment']['type'];
    $form['course_session'][$assignment_type][$assignment_type . '_subject']['#default_value'] = $form_state['send_info']['subject'];
    $form['course_session'][$assignment_type][$assignment_type . '_message']['#default_value'] = $form_state['send_info']['message'];
    $form['course_session']['summary_emails']['#default_value'] = $form_state['send_info']['summary_emails'];
    $form['course_session']['assistant_email']['#default_value'] = $form_state['send_info']['assistant_email'];

    $participants = _fa_course_get_participants_for_session($course_session);
    return confirm_form(
      $form,
      t('Are you sure you want to send the message with the subject "%subject" to %participants participants?',
          array('%subject' => $form_state['send_info']['is_reminder'] ? t("Reminder: ") . $form_state['send_info']['subject'] : $form_state['send_info']['subject'],
                '%participants' => $participants)),
      "admin/people/courses/sessions/" . $form_state['course_session']->facid . '/edit/' . $form_state['course_session']->facsid,
      t('This action cannot be undone.'),
      t('Send'),
      t('Cancel')
    );
  }
}

function _fa_course_get_participants_for_session($course_session){
  return db_query("SELECT COUNT(facpid) FROM fa_course_participant WHERE facid = :facid", array(':facid' => $course_session->facid))->fetchField();
}

function _fa_course_get_assignment_form($course_session, $assignment, $assignment_options, $product_name) {

  $form = array();

  $assignment_sent_datetime = $assignment['type'] . '_sent';
  $assignment_sent = isset($course_session->$assignment_sent_datetime);

  $form[$assignment['type']] = array(
      '#type' => 'fieldset',
      '#title' => ucfirst($assignment['name']),
  );

  $assignment_type_node_id = $assignment['type'] . '_node_id';
  $form[$assignment['type']][$assignment['type'] . '_check'] = array(
      '#type'           => 'checkbox',
      '#title'          => t('This session has a ' . $assignment['name']),
      '#disabled'       => $assignment_sent || empty($assignment_options),
      '#default_value'  => isset($course_session->$assignment_type_node_id) ? TRUE : FALSE,
  );

  $form[$assignment['type']][$assignment['type'] . '_select'] = array(
      '#type' => 'select',
      '#title' => t(ucfirst($assignment['name']) . ' form'),
      '#default_value' => isset($course_session->$assignment_type_node_id) ?
                                  $course_session->$assignment_type_node_id :
                                    reset($assignment_options),
      '#options' => $assignment_options,
      '#disabled' => $assignment_sent,
      '#states' => array(
          'visible' => array(
              ':input[name=' . $assignment['type'] . '_check]' => array('checked' => TRUE),
          ),
      ),
  );

  $form[$assignment['type']] = $form[$assignment['type']]
            + _fa_course_get_email_template($course_session, $assignment, $assignment_sent, $product_name);

  $assignment_datetime = $assignment['type'] . '_datetime';
  $form[$assignment['type']][$assignment['type'] . '_action'] = array(
      '#type'           => 'radios',
      '#title'          => t(ucfirst($assignment['name']) . ' action'),
      '#access'         => !$assignment_sent,
      '#options'        => array(SCHEDULE => t('Schedule ' . $assignment['name']),
                                 SEND_NOW => t('Send ' . $assignment['name'] . ' now')),
      '#default_value'  => isset($course_session->$assignment_datetime) ?
                            SCHEDULE : NULL,
      '#states' => array(
          'visible' => array(
              ':input[name=' . $assignment['type'] . '_check]' => array('checked' => TRUE),
          ),
      ),
  );

  $form[$assignment['type']][$assignment['type'] . '_send_now'] = array(
      '#type' => 'submit',
      '#value' => t('Send now'),
      '#submit' => array('fa_course_session_send_message'),
      '#states' => array(
          'visible' => array(
              ':input[name=' . $assignment['type'] . '_check]' => array('checked' => TRUE),
              ':input[name=' . $assignment['type'] . '_action]' => array('value' => SEND_NOW),
              ),
      ),
  );

  $default_assignment_datetime = date('Y-m-d H:i', REQUEST_TIME);
  if (isset($course_session->session_start_datetime)){
    $default_assignment_datetime = date('Y-m-d H:i', $course_session->session_start_datetime - (SECONDS_IN_DAY * 14));
  }

  $form[$assignment['type']][$assignment['type'] . '_schedule_datetime'] = array(
      '#type'           => 'date_popup',
      '#title'          => t('Date and time when the ' .  $assignment['name'] . ' should be sent'),
      '#access'         => !$assignment_sent,
      '#date_format'    => DATE_FORMAT,
      '#default_value'  => isset($course_session->$assignment_datetime) ? date('Y-m-d H:i', $course_session->$assignment_datetime) :
                                 $default_assignment_datetime,
      '#states' => array(
          'visible' => array(
              ':input[name=' . $assignment['type'] . '_check]' => array('checked' => TRUE),
              ':input[name=' . $assignment['type'] . '_action]' => array('value' => SCHEDULE),
              ),
      ),
  );

  $form[$assignment['type']]['sent'] = array(
      '#type' => 'textfield',
      '#title' => t(ucfirst($assignment['name']) . ' sent time'),
      '#disabled' => TRUE,
      '#access' => $assignment_sent,
      '#value' => isset($course_session->$assignment_sent_datetime) ?
                      format_date($course_session->$assignment_sent_datetime, 'custom', DATE_FORMAT) : NULL,
      '#states' => array(
          'visible' => array(
              ':input[name=' . $assignment['type'] . '_check]' => array('checked' => TRUE),
          ),
      ),
  );

  $reminder_sent_datetime = $assignment['type'] . '_reminder_sent';
  $assignment_reminder_datetime = $assignment['type'] . '_reminder_datetime';
  $form[$assignment['type']][$assignment['type'] . '_reminder_action'] = array(
      '#type'           => 'radios',
      '#title'          => t(ucfirst($assignment['name']) . ' reminder action'),
      '#access'         => !isset($course_session->$reminder_sent_datetime),
      '#options'        => array(SCHEDULE => t('Schedule ' . $assignment['name'] . ' reminder'),
                                 SEND_NOW => t('Send ' . $assignment['name'] . ' reminder now')),
      '#default_value'  => isset($course_session->$assignment_reminder_datetime) ?
                            SCHEDULE : NULL,
      '#states' => array(
          'visible' => array(
              ':input[name=' . $assignment['type'] . '_check]' => array('checked' => TRUE),
          ),
      ),
  );

  $form[$assignment['type']][$assignment['type'] . '_reminder_send_now'] = array(
      '#type' => 'submit',
      '#value' => t('Send now'),
      '#submit' => array('fa_course_session_send_message'),
      '#states' => array(
          'visible' => array(
              ':input[name=' . $assignment['type'] . '_check]' => array('checked' => TRUE),
              ':input[name=' . $assignment['type'] . '_reminder_action]' => array('value' => SEND_NOW),
              ),
      ),
  );

  $default_reminder_datetime = date('Y-m-d H:i', REQUEST_TIME);
  if (isset($course_session->session_start_datetime)){
    $default_reminder_datetime = date('Y-m-d H:i', $course_session->session_start_datetime - (SECONDS_IN_DAY * 2));
  }

  $form[$assignment['type']][$assignment['type'] . '_reminder_schedule_datetime'] = array(
      '#type'           => 'date_popup',
      '#title'          => t('Date and time when a reminder for the ' .  $assignment['name'] . ' should be sent'),
      '#access'         => !isset($course_session->$reminder_sent_datetime),
      '#date_format'    => DATE_FORMAT,
      '#default_value'  => isset($course_session->$assignment_datetime) ? date('Y-m-d H:i', $course_session->$assignment_reminder_datetime) :
                                 $default_reminder_datetime,
      '#states' => array(
          'visible' => array(
              ':input[name=' . $assignment['type'] . '_check]' => array('checked' => TRUE),
              ':input[name=' . $assignment['type'] . '_reminder_action]' => array('value' => SCHEDULE),
              ),
      ),
  );

  $form[$assignment['type']]['reminder_sent'] = array(
      '#type' => 'textfield',
      '#title' => t(ucfirst($assignment['name']) . ' reminder sent time'),
      '#disabled' => TRUE,
      '#access' => isset($course_session->$reminder_sent_datetime),
      '#value' => isset($course_session->$assignment_reminder_datetime) ?
                      format_date($course_session->$assignment_reminder_datetime, 'custom', DATE_FORMAT) : NULL,
      '#states' => array(
          'visible' => array(
              ':input[name=' . $assignment['type'] . '_check]' => array('checked' => TRUE),
          ),
      ),
  );

  return $form;
}

function _fa_course_get_email_template($course_session, $assignment, $assignment_sent, $product_name){

  $form = array();

  $template = array();
  $assignment_email_template = $assignment['type'] . '_email_template';
  if (isset($course_session->$assignment_email_template))
    $template = unserialize($course_session->$assignment_email_template);

  $default_subject_id = 'fa_course_default_' . strtolower($product_name) .'_'. $assignment['type'] . '_subject_template';
  $form[$assignment['type'] . '_subject'] = array(
      '#type'           => 'textfield',
      '#title'          => t(ucfirst($assignment['name']) . ' email title'),
      '#default_value'  => !empty($template) ? $template['subject'] :
            variable_get($default_subject_id, NULL),
      '#disabled'       => $assignment_sent,
      '#states' => array(
          'visible' => array(
              ':input[name=' . $assignment['type'] . '_check]' => array('checked' => TRUE),
          ),
      ),
  );

  $default_message_id = 'fa_course_default_' . strtolower($product_name) .'_'. $assignment['type'] . '_message_template';
  $form[$assignment['type'] . '_message'] = array(
      '#type'           => 'textarea',
      '#title'          => t(ucfirst($assignment['name']) . ' email body'),
      '#default_value'  => !empty($template) ? $template['message'] :
            variable_get($default_message_id, NULL),
      '#disabled'       => $assignment_sent,
      '#states' => array(
          'visible' => array(
              ':input[name=' . $assignment['type'] . '_check]' => array('checked' => TRUE),
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
  $preliminary_assignment = unserialize(PRELIMINARY_ASSIGNMENT);
  _fa_course_validate_assignment_schedule($form_state, $preliminary_assignment);

  // Validate that preliminary assignment has invite token
  $preliminary_assignment_message = $preliminary_assignment['type'] . '_message';
  if (!empty($form_state['values'][$preliminary_assignment_message])){
    if (strpos($form_state['values'][$preliminary_assignment_message], '[invite:join-link]') == FALSE){
      form_set_error($preliminary_assignment_message, t('Preliminary assignment has to have an [invite:join-link] token.'));
    }
  }

  // Course assignment
  _fa_course_validate_assignment_schedule($form_state, unserialize(COURSE_ASSIGNMENT));

  // Feedback
  _fa_course_validate_assignment_schedule($form_state, unserialize(FEEDBACK));

}

function _fa_course_validate_assignment_schedule($form_state, $assignment){
  if ($form_state['values'][$assignment['type'] . '_action'] == SCHEDULE){
    $assignment_datetime = strtotime($form_state['values'][$assignment['type'] . '_schedule_datetime']);
    if ($assignment_datetime < strtotime("now")){
      form_set_error($assignment['type'] . '_schedule_datetime', t('Schedule must be in the future.'));
    }
  }

  if ($form_state['values'][$assignment['type'] . '_reminder_action'] == SEND_NOW){
    $assignment_sent = $assignment['type'] . '_sent';
    if (!isset($form_state['course_session']->$assignment_sent)){
      form_set_error($assignment['type'] . '_reminder_action', t('Can not send reminder before assignment!'));
    }
  }elseif($form_state['values'][$assignment['type'] . '_reminder_action'] == SCHEDULE){
    $reminder_datetime = strtotime($form_state['values'][$assignment['type'] . '_reminder_schedule_datetime']);
    if ($reminder_datetime < strtotime("now")) {
      form_set_error($assignment['type'] . 'reminder_schedule_datetime', t('Schedule must be in the future.'));
    }
    if (isset($assignment_datetime) && $reminder_datetime < $assignment_datetime){
      form_set_error($assignment['type'] . '_reminder_schedule_datetime', t('Reminder schedule must be after the assignment schedule.'));
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

  _fa_course_store_assignment($course_session, $form_state, unserialize(PRELIMINARY_ASSIGNMENT));
  _fa_course_store_assignment($course_session, $form_state, unserialize(COURSE_ASSIGNMENT));
  _fa_course_store_assignment($course_session, $form_state, unserialize(FEEDBACK));
  _fa_course_store_assignment($course_session, $form_state, unserialize(FOLLOWUP));

  // Send emails
  if (isset($form_state['send_info'])){
    module_load_include('inc', 'fa_course', 'fa_course.email');
    $number_of_emails = fa_course_send_assignment($course_session, $form_state['send_info']['assignment']);

    if ($number_of_emails > 0){
      // If course role has not been created create it now
      if(_fa_course_set_course_role($course_session->facid) == FALSE){
        drupal_set_message(t('Could not create course role.'), 'error');
      }

      // Set assignment sent
      $assignment_sent = $form_state['send_info']['assignment']['type'] . '_sent';
      $course_session->$assignment_sent = REQUEST_TIME;
      drupal_set_message(t('@num emails sent to participants.', array('@num' => $number_of_emails)));
    }else{
      drupal_set_message(t('Error sending emails.', array('@facsid' => $course_session->facsid)), 'error');
    }
  }

  if (isset($course_session->facsid))
    drupal_write_record('fa_course_session', $course_session, 'facsid');
  else
    drupal_write_record('fa_course_session', $course_session);
  drupal_set_message(t('Course session @facsid saved.', array('@facsid' => $course_session->facsid)));

  $form_state['redirect'] = "admin/people/courses/sessions/" . $course_session->facid;
}

function _fa_course_store_assignment($course_session, $form_state, $assignment){
  // Save assignments only if they haven't been sent already
  $assignment_sent = $assignment['type'] . '_sent';

  if (!isset($course_session->$assignment_sent)){

    // Either checked manually on save or during sending, when checked flag for some reason can not be set
    if ($form_state['values'][$assignment['type'] . '_check'] ||
        (isset($form_state['send_info']) && $form_state['send_info']['assignment']['type'] == $assignment['type'])){
      $template = array();
      $template['subject'] = $form_state['values'][$assignment['type'] . '_subject'];
      $template['message'] = $form_state['values'][$assignment['type'] . '_message'];
      $assignment_email_template = $assignment['type'] . '_email_template';
      $course_session->$assignment_email_template = serialize($template);

      if ($assignment['type'] != 'followup'){
        $assignment_node_id = $assignment['type'] . '_node_id';
        $course_session->$assignment_node_id = $form_state['values'][$assignment['type'] . '_select'];

        if (isset($form_state['values'][$assignment['type'] . '_action'])){
          if ($form_state['values'][$assignment['type'] . '_action'] == SCHEDULE){
            $assignment_datetime = $assignment['type'] . '_datetime';
            $course_session->$assignment_datetime = strtotime($form_state['values'][$assignment['type'] . '_schedule_datetime']);
            // TODO: schedule using the  Rules module!
          }
        }
      }
    }
  }
}

function fa_course_session_send_message($form = array(), &$form_state = array()){

  if ($form_state['values']['preliminary_assignment_action'] == SEND_NOW){
    $send_info = _fa_course_get_send_info($form_state, unserialize(PRELIMINARY_ASSIGNMENT));
  }elseif($form_state['values']['preliminary_assignment_reminder_action'] == SEND_NOW){
    $send_info = _fa_course_get_send_info($form_state, unserialize(PRELIMINARY_ASSIGNMENT_REMINDER));
  }elseif($form_state['values']['course_assignment_action'] == SEND_NOW){
    $send_info = _fa_course_get_send_info($form_state, unserialize(COURSE_ASSIGNMENT));
  }elseif($form_state['values']['course_assignment_reminder_action'] == SEND_NOW){
    $send_info = _fa_course_get_send_info($form_state, unserialize(COURSE_ASSIGNMENT_REMINDER));
  }elseif($form_state['values']['feedback_action'] == SEND_NOW){
    $send_info = _fa_course_get_send_info($form_state, unserialize(FEEDBACK));
  }elseif($form_state['values']['feedback_reminder_action'] == SEND_NOW){
    $send_info = _fa_course_get_send_info($form_state, unserialize(FEEDBACK_REMINDER));
  }elseif($form_state['values']['followup_action'] == TRUE){
    $send_info = _fa_course_get_send_info($form_state, unserialize(FOLLOWUP));
  }

  if (isset($send_info)){
    $form_state['send_info'] = $send_info;
    $form_state['storage']['confirm'] = TRUE; // this will cause the form to be rebuilt, entering the confirm part of the form
    $form_state['rebuild'] = TRUE; // along with this
  }
}

function _fa_course_get_send_info($form_state, $assignment){

  $reminder_len = strlen('_reminder');
  if ($assignment['is_reminder']){
    $subject = $form_state['values'][substr($assignment['type'], strlen($assignment['type']) - $reminder_len) . '_subject'];
    $message = $form_state['values'][substr($assignment['type'], strlen($assignment['type']) - $reminder_len) . '_message'];
  }
  else{
    $subject = $form_state['values'][$assignment['type'] . '_subject'];
    $message = $form_state['values'][$assignment['type'] . '_message'];
  }
  return array('assignment' => $assignment,
               'subject' => $subject,
               'message' => $message,
               'summary_emails' => $form_state['values']['summary_emails'],
               'assistant_email' => $form_state['values']['assistant_email'],
               'is_reminder' => $assignment['is_reminder']);
}

function fa_course_sessions_delete($facid, $facsid){
  $links = '<p>' . l(t("Back to sessions"), "admin/people/courses/sessions/$facid") . '</p>';
  $output = render($links);
  $session = _fa_course_get_course_session($facid, $facsid);
  $delete_form = drupal_get_form('fa_course_sessions_delete_form', $session);
  $output .= render($delete_form);
  return $output;
}

function fa_course_sessions_delete_form($form, &$form_state, $session) {
  $form['session'] = array(
    '#type' => 'value',
    '#value' => $session,
  );

  $form_state['rebuild'] = TRUE;
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
