<?php

/**
 * @file
 * Email functions for Filosofian Akatemia Course module.
 */

// Permitted actions
define('SEND', 1);
define('ADD_EMAIL', 2);
define('FIX_EMAIL', 3);

function send_assignment($course_session, $assignment, $action, $participant = NULL, $old_email = NULL){

  $email_template = _get_email_template($course_session, $assignment);
  $emails = _get_emails($course_session, $email_template, $action, $participant);
  foreach ($emails as $email){
    // Send email
    _send_email($email);
  }

  _send_summary($email_template, $emails, $course_session->summary_emails, $action, $participant, $old_email);
}

function _get_email_template($course_session, $assignment){
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

function _get_emails($course_session, $email_template, $action, $new_email){
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

  if ($action == SEND){
    // First get all participants for the session
    $result = db_select('fa_course_participant', 'p')
                ->fields('p', array('email','invite_reg_code'));
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
    if (strpos($email_template, '[invite:join-link]') == FALSE){
      $one_email = TRUE;
    }
    foreach ($result as $row) {
      if ($one_email){
        // Just create one big bcc address
        if (empty($emails)){
          $email = _get_email_array($row, $email_template, $from);
          $email['to'] = variable_get('fa_course_default_to_email', 'info@filosofianakatemia.fi');
          $email['bcc'] = $row->email;
          array_push($emails, $email);
        }else{
          // Add participant email to the end of the address
          $email = array_shift(array_values($emails));
          $email['bcc'] = $email['bcc'] . ', ' . $row->email;
        }
      }else{
        $email = _get_email_array($row, $email_template, $from);
        $email['to'] = $row->email;
        array_push($emails, $email);
      }
    }
  }elseif ($action == ADD_EMAIL || $action == FIX_EMAIL){
    $email = _get_email_array($participant, $email_template, $from);
    $email['to'] = $participant->email;
    array_push($emails, $email);
  }
  return $emails;
}

function _get_email_array($participant, $email_template, $from){
  $email_content = _get_email_content($email_template, $participant);
  return array(
        'from' => $from,
        'subject' => $email_content['subject'],
        'message' => $email_content['message']
      );
}

function _get_lead_trainer_uid($course_session){
  return db_query("SELECT lead_trainer_id FROM fa_course WHERE facid = :facid", array(':facid' => $course_session->facid))->fetchField();

}

function _get_email_content($email_template, $participant){
  return $email = $email_template['message']
            .str_replace('[invite:join-link]',
               url('invite/accept/' . $participant->invite_reg_code, array('absolute' => TRUE)));
}

function _send_summary($email_template, $emails, $summary_emails, $action, $participant, $old_email){
  $summary = array();
  if ($action == SEND){
    $summary['subject'] = t('SUMMARY: ') . $email_template['subject'];
    $summary['message'] = t("An invitation using the following message template was sent:" . "\n\n")
                          . 'from: ' . $email['from'] . '\n';
    if (count($emails) == 1){
      $summary['message'] .= 'to: ' . $email['to'] . '\n'
                           . 'bcc: ' . $email['bcc'];
    }else{
      $summary['message'] .= 'to:' . "\n";
      foreach($emails as $email){
        $summary['message'] .= '    ' . $email['to'] . "\n";
      }
    }
  }elseif ($action == ADD_EMAIL){
    $summary['subject'] = t('ADDED: ') . $email_template['subject'];
    $summary['message'] = t('An invitation like following was sent to this additional recipient:' . "\n\n")
                            . $participant->email . "\n";
  }elseif ($action == FIX_EMAIL){
    $summary['subject'] = t('FIXED: ') . $email_template['subject'];
    $summary['message'] = t("The following message was sent to the fixed email address:" . "\n\n")
                       . $old_email . ' => ' . $participant->email . "\n";
  }
  $summary['message'] .= "\n**********\n\n"
                     . trim($email_template['subject']) . "\n\n"
                     . $email_template['message'];

  $summary['from'] = variable_get('fa_course_default_from_email', 'webmaster@filosofianakatemia.fi');
  $summary['to'] = $summary_emails;
  _send_email($summary);
}


function _send_email($email){
  global $language;
  $result = drupal_mail('fa_course', 'fa_course', $email['to'], $language, $email, $email['from'], TRUE);
}

/**
 * Implements hook_mail().
 */
function fa_course_mail($key, &$message, $email) {
  if (isset($email['subject'])) $message['subject'] = $email['subject'];
  if (isset($email['message'])) $message['body'][] = $email['message'];
  if (!empty($email['reply-to'])) {
    $message['headers']['Reply-To'] = $email['reply-to'];
  }
}