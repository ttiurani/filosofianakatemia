<?php

/**
 * @file
 * Email functions for Filosofian Akatemia Course module.
 */


function send_assignment($course_session, $assignment){


  $subject = $course_session->$assignment_template;


  // Summary emails
  $summary = array(
      'subject' => t("SUMMARY: ") . trim($subject),
      'message' => t("Message using the following template was sent to these recipients:" . "\n\n"),
  );

  foreach ($form_state['values']['remind_invites'] as $remind_invite){
    if (!isset($remind_invite_subject) && !isset($remind_invite_message)){
      $remind_invite_subject = $remind_invite->data['subject'];
      $remind_invite_message = str_replace($remind_invite->reg_code, '[invite:reg-code]', $remind_invite->data['message']);
      $summary['subject'] .= ' ' . $remind_invite_subject;
    }
    $summary['message'] .= $remind_invite->email . "\n";
  }
  $summary['message'] .= "\n**********\n\n";
  $summary['message'] .= trim($remind_invite_subject) . "\n\n";
  $summary['message'] .=  $remind_invite_message;

  $from = variable_get('fa_course_default_from_email', 'webmaster@filosofianakatemia.fi');


  global $language;
  // Send the summary
  $result = send_email($from, $to, $cc, $bcc, $subject, $message);
}

function send_email($from, $to, $cc, $bcc, $subject, $message){
  $result = drupal_mail('fa_course', 'fa_course', $email_header, $language, $email_content, $from, TRUE);
}

/**
 * Implements hook_mail().
 */
function fa_course_mail($key, &$message, $email_content) {
  global $user;
  if (isset($summary['subject'])) $message['subject'] = $summary['subject'];
  if (isset($summary['message'])) $message['body'][] = $summary['message'];
}