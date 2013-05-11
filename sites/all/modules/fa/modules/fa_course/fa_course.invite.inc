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

  $status = _fa_course_get_invite_status($participant);

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

/**
 * Checks the status of an invite.
 */
function _fa_course_get_invite_status($participant) {

  if (isset($participant->invite_joined)){
    if(isset($participant->invite_reuse) && $participant->reuse > 0) {
      return INVITE_REUSABLE;
    }
    return INVITE_USED;
  }
  return INVITE_VALID;
}

/**
 * Implements hook_user_insert().
 */
function fa_course_user_insert(&$edit, $account, $category) {
  if (isset($_SESSION) && isset($_SESSION[INVITE_SESSION]) && isset($_SESSION[INVITE_SESSION_STATUS])){
    _fa_course_process_invite($_SESSION[INVITE_SESSION], $account, $_SESSION[INVITE_SESSION_STATUS]);
    _fa_course_clear_session();
  }
}

/**
 * Implements hook_user_login()
  */
function fa_course_user_login(&$edit, $account){
  if (isset($_SESSION) && isset($_SESSION[INVITE_SESSION]) && isset($_SESSION[INVITE_SESSION_STATUS])){
    _fa_course_process_invite($_SESSION[INVITE_SESSION], $account, $_SESSION[INVITE_SESSION_STATUS]);
    _fa_course_clear_session();
  }
}

function _fa_course_process_invite($reg_code, $account, $status){
  if ($status != INVITE_USED){
    if ($status == INVITE_VALID){
      $participant = fa_course_participant_load($reg_code);
      db_query("UPDATE fa_course_participant SET uid = :uid, invite_joined = :joined "
              ."WHERE reg_code = :reg_code", array(':uid' => $account->uid, ':joined' => REQUEST_TIME, ':reg_code' => $participant->invite_reg_code));
    }elseif($status == INVITE_REUSABLE){
      $master_participant = fa_course_participant_load($reg_code);
      db_query("UPDATE {fa_course_participant} SET invite_reuse = invite_reuse - 1 WHERE facpid = :facpid", array(':facpid' => $master_participant->facpid));
      $participant = new stdClass();
      $participant->facid = $master_participant->facid;
      $participant->email = $account->mail;
      drupal_write_record('fa_course_participant', $participant);
    }

    // Process roles
    $new_roles = _fa_course_get_new_roles($participant);
    if (!$new_roles){
      // Save roles, skips if already set
      foreach ($new_roles as $new_role) {
        user_multiple_role_edit(array($account->uid), 'add_role', $new_role);
      }
    }
  }
}

function _fa_course_get_new_roles($participant){
  $roles = array();
  $result = db_query("SELECT c.product_id, c.organization_id, c.course_id FROM fa_course c WHERE c.facid = :facid",
           array(':facid' => $participant->facid));
  foreach($result as $row){
    $roles[$row->product_id] = $row->product_id;
    $roles[$row->organization_id] = $row->organization_id;
    $roles[$row->course_id] = $row->course_id;
  }
  return $roles;
}

function _fa_course_clear_session(){
  if (isset($_SESSION)) {
    unset($_SESSION[INVITE_SESSION]);
    if (isset($_SESSION[INVITE_SESSION_STATUS]))
      unset($_SESSION[INVITE_SESSION_STATUS]);
    if (isset($_SESSION[INVITE_SESSION_DESTINATION]))
      unset($_SESSION[INVITE_SESSION_DESTINATION]);
    if (isset($_SESSION[INVITE_SESSION_EMAIL]))
      unset($_SESSION[INVITE_SESSION_EMAIL]);
  }
}
