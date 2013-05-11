<?php

/**
 * @file
 * Email functions for Filosofian Akatemia Course module.
 */

// Permitted actions
define('SEND', 1);
define('REMIND', 2);
define('ADD_EMAIL', 3);
define('FIX_EMAIL', 4);

function fa_course_send_assignment_email($course_session, $assignment){
  if ($assignment['is_reminder']) $action = REMIND;
  else $action = SEND;

  $email_template = _fa_course_get_assignment_email_template($course_session, $assignment);
  $emails = _fa_course_get_assignment_emails($course_session, $email_template, $action);
  if (!empty($emails)){
    foreach ($emails as $email){
      // Send email
      $message = _fa_course_send_email($email);
      if ($message == NULL){
        return -1;
      }
      // Save email
      _fa_course_save_email($course_session->facsid, $assignment['id'], $message, $email['participant_ids']);
    }
    _fa_course_send_summary($course_session->facid, $email_template, $emails, $course_session->summary_emails, $action);
  }
  return count($emails);
}

function fa_course_fix_email($participant, $old_email){
  return _fa_course_alter_email(FIX_EMAIL, $participant, $old_email);
}

function fa_course_add_email($participant){
  return _fa_course_alter_email(ADD_EMAIL, $participant);
}

function _fa_course_alter_email($action, $participant, $old_email = NULL){
  $first_session = _fa_course_get_first_session($participant->facid);
  if ($first_session != NULL && $first_session->session_start_datetime > REQUEST_TIME && isset($first_session->preliminary_assignment_sent)){
    // Preliminary assignment has already been sent, need to send it again
    $assignment = unserialize(PRELIMINARY_ASSIGNMENT);
    $email_template = _fa_course_get_assignment_email_template($first_session, $assignment);
    $email = _fa_course_get_assignment_emails($first_session, $email_template, $action, $participant);
    // Send email
    $message = _fa_course_send_email($email);
    if ($message == FALSE){
      return -1;
    }
    // Save email
    _fa_course_save_email($first_session->facsid, $assignment['id'], $message, $email['participant_ids']);
    _fa_course_send_summary($first_session->facid, $email_template, array($email), $first_session->summary_emails, $action, $participant, $old_email);
    return 1;
  }
  return 0;
}


function fa_course_send_trainer_handover($course){
  return _fa_course_send_handover(TRUE, $course);
}

function fa_course_send_sales_handover($course){
  return _fa_course_send_handover(FALSE, $course);
}

function _fa_course_send_handover($is_trainer, $course){
  // Get emails
  $result = db_query("SELECT trainer.mail AS trainer_email, sales.mail AS sales_email FROM fa_course c "
          ."INNER JOIN users trainer ON c.lead_trainer_id = trainer.uid "
          ."INNER JOIN users sales ON c.lead_sales_id = sales.uid "
          ."WHERE c.facid = :facid", array(':facid' => $course->facid)
     );
  foreach($result as $row){
    $sales_email = $row->sales_email;
    $trainer_email = $row->trainer_email;
    break;
  }

  // Create email
  $email = array(
    'to' => $is_trainer ? $trainer_email : $sales_email,
    'cc' => $is_trainer ? $sales_email : $trainer_email,
    'from' => variable_get('fa_course_default_from_email', 'webmaster@filosofianakatemia.fi'),
  );

  $course_name = _fa_course_get_course_name($course->facid);

  if ($is_trainer){
    $email['subject'] = t("TRAINER HANDOVER: ");
    $email['message'] = t("@sales_email handed over the course @course_name to the trainer @trainer_email.",
                           array('@sales_email' => $sales_email, '@course_name' => $course_name, '@trainer_email' => $trainer_email))
                           . "\n\n";
  }else{
    $email['subject'] = t("SALES HANDOVER: ");
    $email['message'] = t("@trainer_email handed back over the course @course_name to the lead sales @sales_email.",
                           array('@sales_email' => $sales_email, '@course_name' => $course_name, '@trainer_email' => $trainer_email))
                           . "\n\n";
  }
  $email['subject'] .= $course_name;
  $email['message'] .= t("View the course at: ") . url('admin/people/courses/view/' . $course->facid, array('absolute' => TRUE));

  // Send email
  $message = _fa_course_send_email($email);
  if ($message == NULL){
    return FALSE;
  }
  return TRUE;
}

function fa_course_send_billing_email($facid, $sum, $billing_information, $trainer_email, $sales_email){
  // Create email
  $email = array(
    'to' => variable_get('fa_course_billing_emails', NULL),
    'cc' => $trainer_email . ', ' . $sales_email,
    'from' => variable_get('fa_course_default_from_email', 'webmaster@filosofianakatemia.fi'),
  );

  $course_name = _fa_course_get_course_name($facid);

  $email['subject'] = t("READY FOR BILLING: ") . $course_name;
  $email['message'] = t("The first session of the @course_name has been held and the course is now ready to be billed.",
                           array('@course_name' => $course_name))
                           . "\n\n";
  $email['message'] .= t("Sum from CapsuleCRM: ") . $sum . "\n";
  $email['message'] .= t("Billing information: ") . $billing_information . "\n\n";
  $email['message'] .= t("View the course at: ") . url('admin/people/courses/view/' . $facid, array('absolute' => TRUE));

  // Send email
  $message = _fa_course_send_email($email);
  if ($message == NULL){
    return FALSE;
  }
  return TRUE;
}

function _fa_course_get_assignment_email_template($course_session, $assignment){
  switch ($assignment['id']) {
    case 1:
      $email_template = unserialize($course_session->preliminary_assignment_email_template);
      break;
    case 2:
      $email_template = unserialize($course_session->preliminary_assignment_email_template);
      $email_template['subject'] = t('Reminder: ') . $email_template['subject'];
      break;
    case 3:
      $email_template = unserialize($course_session->course_assignment_email_template);
      break;
    case 4:
      $email_template = unserialize($course_session->course_assignment_email_template);
      $email_template['subject'] = t('Reminder: ') . $email_template['subject'];
      break;
    case 5:
      $email_template = unserialize($course_session->followup_email_template);
      break;
    case 6:
      $email_template = unserialize($course_session->feedback_email_template);
      break;
    case 7:
      $email_template = unserialize($course_session->feedback_email_template);
      $email_template['subject'] = t('Reminder: ') . $email_template['subject'];
      break;
    default:
      drupal_set_message(t('Unsupported assignment id @id.',  array('@id' => $assignment['id'])), 'error');
      return;
  }

  if ($assignment['is_reminder']){
    $original_assignment_type = substr($assignment['type'], 0, strlen($assignment['type']) - strlen('_reminder'));
    $assignment_node_id = $original_assignment_type . '_node_id';
  }else{
    $assignment_node_id = $assignment['type'] . '_node_id';
  }
  if (isset($course_session->$assignment_node_id)){
    $email_template['message'] = str_replace('[invite:join-link]',
                                   '[invite:join-link]?destination='
                                      . _fa_course_get_node_url($course_session->$assignment_node_id),
                                   $email_template['message']);
  }
  return $email_template;
}

function _fa_course_get_assignment_emails($course_session, $email_template, $action, $participant = NULL){
  $sender_fields = user_load(_fa_course_get_lead_trainer_info($course_session->facid)->lead_trainer_id);

  if (!empty($sender_fields->field_first_name) && !empty($sender_fields->field_last_name)){
    $firstname = $sender_fields->field_first_name['und']['0']['value'];
    $lastname = $sender_fields->field_last_name['und']['0']['value'];
    $from = $firstname . ' ' . $lastname . ' <' . $sender_fields->mail . '>';
  }else{
    $from = $sender_fields->mail;
  }

  $emails = array();

  if ($action == SEND || $action == REMIND){
    // First get all participants for the session
    $result = db_select('fa_course_participant', 'p')
                  ->fields('p', array('facpid', 'email','invite_reg_code'));
    $result->condition('p.facid', $course_session->facid);

    if ($action == REMIND){
      // Add additional condition to exclude participants who have already used invite
      $result->condition('p.invite_joined', NULL);
    }
    $result->distinct();
    $result = $result->execute()->fetchAll();

    if (empty($result)){
      return array();
    }

    // Check whether emails need to be sent one at a time, or as one big email
    $one_email = NULL;
    if (strpos($email_template['message'], '[invite:join-link]') == FALSE){
      $one_email = array();
    }

    foreach ($result as $row) {
      $participant_email = $row->email;
      if (is_array($one_email)){
        // Just create one big bcc address
        if (empty($one_email)){
          $one_email = _fa_course_get_email_array($row, $email_template, $from);
          $one_email['to'] = variable_get('fa_course_default_to_email', 'info@filosofianakatemia.fi');
          $one_email['bcc'] = $participant_email;
          $one_email['participant_ids'] = array($row->facpid);
        }else{
          // Add participant email to the end of the address
          $one_email['bcc'] .= ', ' . $participant_email;
        }
      }else{
        $email = _fa_course_get_email_array($row, $email_template, $from);
        $email['to'] = $participant_email;
        $email['participant_ids'] = array($row->facpid);
        array_push($emails, $email);
      }
    }

    if (is_array($one_email)) array_push($emails, $one_email);

  }elseif ($action == ADD_EMAIL || $action == FIX_EMAIL){
    $email = _fa_course_get_email_array($participant, $email_template, $from);
    $email['to'] = $participant->email;
    $email['participant_ids'] = array($participant->facpid);
    return $email;
  }

  return $emails;
}

function _fa_course_get_email_array($participant, $email_template, $from){
  $email_content = _fa_course_get_email_content($email_template, $participant);
  return array(
        'from' => $from,
        'reply-to' => $from,
        'subject' => $email_content['subject'],
        'message' => $email_content['message']
      );
}

function _fa_course_get_email_content($email_template, $participant){
  $email_content['subject'] = $email_template['subject'];
  $email_content['message'] = str_replace('[invite:join-link]',
                                 url('invite/accept/' . $participant->invite_reg_code,
                                      array('absolute' => TRUE)),
                                 $email_template['message']);
  return $email_content;
}

function _fa_course_get_node_url($assignment_node_id){
  return db_query("SELECT ua.alias FROM url_alias ua WHERE ua.source = CONCAT('node/', :assignment_node_id)",
                  array(':assignment_node_id' => $assignment_node_id)
                        )->fetchField();
}

function _fa_course_send_summary($facid, $email_template, $emails, $summary_emails, $action, $participant = NULL, $old_email = NULL){
  $summary = array();
  if ($action == SEND || $action == REMIND){

    if ($action == SEND){
      $summary['subject'] = t('SUMMARY: ') . _fa_course_get_course_name($facid);
      $summary['message'] = t("An invitation using the following message template was sent:" . "\n\n")
                          . 'from: ';
    }else {
      $summary['subject'] = t('REMINDED: ') . _fa_course_get_course_name($facid);
      $summary['message'] = t("An reminder using the following message template was sent:" . "\n\n")
                          . 'from: ';
    }

    if (count($emails) == 1){
      $aux = array_values($emails);
      $email = array_shift($aux);
      $summary['message'] .= $email['from'] . "\n"
                           .  'to: ' . $email['to'] . "\n";
      if (isset($email['bcc'])){
        $summary['message'] .= 'bcc: ' . $email['bcc'] . "\n";
      }
    }else{
      $missing_header = TRUE;
      foreach($emails as $email){
        if ($missing_header){
          $summary['message'] .= $email['from'] . "\n"
                            . 'to: ' . $email['to'] . "\n";
          $missing_header = FALSE;
        }else{
          $summary['message'] .= '    ' . $email['to'] . "\n";
        }
      }
    }
  }elseif ($action == ADD_EMAIL){
    $summary['subject'] = t('ADDED: ') . _fa_course_get_course_name($facid);
    $summary['message'] = t('An invitation from the following template was sent to this additional recipient:' . "\n\n")
                            . $participant->email . "\n";
  }elseif ($action == FIX_EMAIL){
    $summary['subject'] = t('FIXED: ') . _fa_course_get_course_name($facid);
    $summary['message'] = t("A message from the following template was sent to the fixed email address:" . "\n\n")
                       . $old_email . ' => ' . $participant->email . "\n";
  }
  $summary['message'] .= "\n**********\n\n"
                     . trim($email_template['subject']) . "\n\n"
                     . $email_template['message'];

  $summary['from'] = variable_get('fa_course_default_from_email', 'Filosofian Akatemia webmaster <webmaster@filosofianakatemia.fi>');
  $summary['to'] = $summary_emails;
  _fa_course_send_email($summary);
}

function _fa_course_send_email($email){
  global $language;
  $message = drupal_mail('fa_course', 'fa_course', $email['to'], $language, $email, $email['from'], TRUE);
  if ($message['result'] != TRUE){
     drupal_set_message(t('Error sending email with subject "@subject"',  array('@subject' => $email['subject'])), 'error');
     return NULL;
  }
  return $message;
}

function _fa_course_save_email($course_session_id, $assignment_id, $message, $participant_ids){
  $facmid = db_insert('fa_course_message')
              ->fields(array(
                'facsid' => $course_session_id,
                'template_type' => $assignment_id,
                'message' => serialize($message),
                'sent' => REQUEST_TIME,
              ))->execute();

  foreach($participant_ids as $participant_id){
    db_insert('fa_participants_messages')
                ->fields(array(
                  'facmid' => $facmid,
                  'facpid' => $participant_id,
                ))->execute();
  }
}

function fa_course_mail($key, &$message, $email) {
  if (isset($email['subject'])) $message['subject'] = $email['subject'];
  if (isset($email['message'])) $message['body'][] = $email['message'];
  if (isset($email['cc'])) $message['headers']['Cc'] = $email['cc'];
  if (isset($email['bcc'])) $message['headers']['Bcc'] = $email['bcc'];
  if (!empty($email['reply-to'])) {
    $message['headers']['Reply-To'] = $email['reply-to'];
  }
}