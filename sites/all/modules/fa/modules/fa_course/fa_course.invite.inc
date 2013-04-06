<?php

/**
 * @file
 * Invite functions for Filosofian Akatemia Course module.
 */


 /**
 * Menu callback; handle incoming requests for accepting an invite.
 */
function fa_course_invite_accept($participant) {
  global $user;

  $status = get_fa_course_invite_status($participant);

  if ($status == INVITE_VALID || $status == INVITE_USED || $status == INVITE_REUSABLE) {
    // Logs out the current user if logged in to avoid problems with setting roles
    if ($user->uid != 0){
      $tmp = NULL;
      session_destroy();
      user_module_invoke('logout', $tmp, $user);
    }

    if (isset($_GET['destination'])){
      $_SESSION[INVITE_SESSION_DESTINATION] = $_GET['destination'];
      unset($_GET['destination']);
    }
    $_SESSION[INVITE_SESSION_EMAIL] = $participant->email;
    $_SESSION[INVITE_SESSION_STATUS] = $status;
    $_SESSION[INVITE_SESSION] = $participant->invite_reg_code;

    if (($status == INVITE_VALID &&
        !db_query("SELECT COUNT(*) FROM {users} WHERE mail = :mail",
          array(':mail' => $participant->email))->fetchField()) ||
        $status == INVITE_REUSABLE){
      drupal_goto('user/register');
    }else{
      drupal_goto('user/login');
    }
  }
}

/**
 * Checks the status of an invite.
 */
function get_fa_course_invite_status($participant) {

  if (isset($participant->invite_joined)){
    if (!isset($participant->invite_reuse) || $participant->invite_reuse == 0) {
      return INVITE_USED;
    }
  }
  elseif (isset($participant->invite_reuse) && $participant->reuse > 0) {
    return INVITE_REUSABLE;
  }

  return INVITE_VALID;
}

