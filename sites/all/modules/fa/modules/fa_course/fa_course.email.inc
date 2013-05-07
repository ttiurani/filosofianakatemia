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

function fa_course_send_assignment($course_session, $assignment){

  if ($assignment['is_reminder']) $action = REMIND;
  else $action = SEND;

  $email_template = _fa_course_get_email_template($course_session, $assignment);
  $emails = _get_emails($course_session, $email_template, $action);
  foreach ($emails as $email){
    // Send email
    $message = _send_email($email);
    if ($message == FALSE){
      return 0;
    }
    // Save email
    _save_email($course_session->facsid, $assignment['id'], $message, $email['participant_ids']);
  }
  _send_summary($course_session->facid, $email_template, $emails, $course_session->summary_emails, $action);
  return count($emails);
}

function fix_email($participant, $old_email){
  $first_session = _get_first_session($participant->facid);
  if ($first_session->session_start_datetime > REQUEST_TIME && isset($first_session->preliminary_assignment_sent)){
    // Preliminary assignment has already been sent, need to send it again
    $email_template = _fa_course_get_email_template($first_session, PRELIMINARY_ASSIGNMENT);
    $email = _get_emails($first_session, $email_template, FIX_EMAIL, $participant);
    // Send email
    $message = _send_email($email);
    if ($message == FALSE){
      return FALSE;
    }
    // Save email
    _save_email($course_session->facsid, $assignment['id'], $message, $email['participant_ids']);
    _send_summary($course_session->facid, $email_template, $emails, $course_session->summary_emails, FIX_EMAIL, $participant, $old_email);
    return TRUE;
  }
  return FALSE;
}

function add_email($participant){
  $first_session = _get_first_session($participant->facid);
  if ($first_session->session_start_datetime > REQUEST_TIME && isset($first_session->preliminary_assignment_sent)){
    $email_template = _fa_course_get_email_template($first_session, PRELIMINARY_ASSIGNMENT);
    $email = _get_emails($first_session, $email_template, ADD_EMAIL, $participant);
    // Send email
    $message = _send_email($email);
    if ($message == FALSE){
      return FALSE;
    }
    // Save email
    _save_email($course_session->facsid, $assignment['id'], $message, $email['participant_ids']);
    _send_summary($course_session->facid, $email_template, $emails, $course_session->summary_emails, ADD_EMAIL, $participant);
    return TRUE;
  }
  return FALSE;
}

function _fa_course_get_email_template($course_session, $assignment){
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
    case 4:
      $email_template = unserialize($course_session->course_assignment_email_template);
      $email_template['subject'] = t('Reminder: ') . $email_template['subject'];
    case 5:
      $email_template = unserialize($course_session->completion_message_email_template);
    case 6:
      $email_template = unserialize($course_session->feedback_email_template);
    case 7:
      $email_template = unserialize($course_session->feedback_email_template);
      $email_template['subject'] = t('Reminder: ') . $email_template['subject'];
    default:
      drupal_set_message(t('Unsupported assignment id @id.',  array('@id' => $assignment['id'])), 'error');
      return;
  }
  return $email_template;
}

function _get_emails($course_session, $email_template, $action, $participant = NULL){
  global $user;

  $sender_fields = user_load(_get_lead_trainer_uid($course_session));

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
    $result->distinct();
    $result = $result->execute()->fetchAll();

    if (empty($result)){
      // No participants, error
      drupal_set_message(t('Session with id @id has no participants.',  array('@id' => $course_session->facsid)), 'error');
      return;
    }

    // Check whether emails need to be sent one at a time, or as one big email
    $one_email = FALSE;
    if (strpos($email_template['message'], '[invite:join-link]') == FALSE){
      $one_email = TRUE;
    }
    foreach ($result as $row) {
      if ($one_email){
        // Just create one big bcc address
        if (empty($emails)){
          $email = _get_email_array($row, $email_template, $from);
          $email['to'] = variable_get('fa_course_default_to_email', 'info@filosofianakatemia.fi');
          $email['bcc'] = $row->email;
          $email['participant_ids'] = array($row->facpid);
          array_push($emails, $email);
        }else{
          // Add participant email to the end of the address
          $email = array_shift(array_values($emails));
          $email['bcc'] = $email['bcc'] . ', ' . $row->email;
          array_push($email['participant_ids'], $row->facpid);
        }
      }else{
        $email = _get_email_array($row, $email_template, $from);
        $email['to'] = $row->email;
        $email['participant_ids'] = array($row->facpid);
        array_push($emails, $email);
      }
    }
  }elseif ($action == ADD_EMAIL || $action == FIX_EMAIL){
    $email = _get_email_array($participant, $email_template, $from);
    $email['to'] = $participant->email;
    $email['participant_ids'] = array($participant->facpid);
    return $email;
  }
  return $emails;
}

function _get_email_array($participant, $email_template, $from){
  $email_content = _get_email_content($email_template, $participant);
  return array(
        'from' => $from,
        'reply-to' => $from,
        'subject' => $email_content['subject'],
        'message' => $email_content['message']
      );
}

function _get_lead_trainer_uid($course_session){
  return db_query("SELECT lead_trainer_id FROM fa_course WHERE facid = :facid", array(':facid' => $course_session->facid))->fetchField();
}

function _get_email_content($email_template, $participant){
  $email_content['subject'] = $email_template['subject'];
  $email_content['message'] = str_replace('[invite:join-link]',
                                 url('invite/accept/' . $participant->invite_reg_code,
                                      array('absolute' => TRUE)),
                                 $email_template['message']);
  return $email_content;
}

function _get_first_session($facid){
  return db_query("SELECT * FROM fa_course_session "
                . "WHERE facid = :facid ORDER BY session_start_datetime ASC LIMIT 1",
                array(':facid' => $course_session->facid))->fetchField();
}

function _send_summary($facid, $email_template, $emails, $summary_emails, $action, $participant = NULL, $old_email = NULL){
  $summary = array();
  if ($action == SEND || $action == REMIND){

    if ($action == SEND) $summary['subject'] = t('SUMMARY: ') . _fa_course_get_course_name($facid);
    else $summary['subject'] = t('REMINDED: ') . _fa_course_get_course_name($facid);

    $summary['message'] = t("An invitation using the following message template was sent:" . "\n\n")
                          . 'from: ';
    if (count($emails) == 1){
      $aux = array_values($emails);
      $email = array_shift($aux);
      $summary['message'] .= $email['from'] . "\n"
                           .  'to: ' . $email['to'] . "\n";
      if (isset($email['bcc'])){
        $summary['message'] .= 'bcc: ' . $email['bcc'] . "\n";
      }
    }else{
      $summary['message'] .= 'to:' . "\n";
      foreach($emails as $email){
        $summary['message'] .= '    ' . $email['to'] . "\n";
      }
    }
  }elseif ($action == ADD_EMAIL){
    $summary['subject'] = t('ADDED: ') . _fa_course_get_course_name($facid);
    $summary['message'] = t('An invitation like following was sent to this additional recipient:' . "\n\n")
                            . $participant->email . "\n";
  }elseif ($action == FIX_EMAIL){
    $summary['subject'] = t('FIXED: ') . _fa_course_get_course_name($facid);
    $summary['message'] = t("The following message was sent to the fixed email address:" . "\n\n")
                       . $old_email . ' => ' . $participant->email . "\n";
  }
  $summary['message'] .= "\n**********\n\n"
                     . trim($email_template['subject']) . "\n\n"
                     . $email_template['message'];

  $summary['from'] = variable_get('fa_course_default_from_email', 'Filosofian Akatemia webmaster <webmaster@filosofianakatemia.fi>');
  $summary['to'] = $summary_emails;
  _send_email($summary);
}

function _send_email($email){
  global $language;
  $message = drupal_mail('fa_course', 'fa_course', $email['to'], $language, $email, $email['from'], TRUE);
  if ($message['result'] != TRUE){
     drupal_set_message(t('Error sending email with subject "@subject"',  array('@subject' => $email['subject'])), 'error');
     return FALSE;
  }
  watchdog('debug', '<pre>'. print_r($message, TRUE) . '</pre>');

  return $message;
}

function _save_email($course_session_id, $assignment_id, $message, $participant_ids){
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
