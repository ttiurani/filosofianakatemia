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
  }else{
    // Do this so to not end up in the unsupported user home
    $_SESSION[INVITE_SESSION_DESTINATION] = 'lykeion';
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

function _fa_course_process_invite($reg_code, $account, $status){
  if ($status != INVITE_USED){
    if ($status == INVITE_VALID){
      $participant = fa_course_participant_load($reg_code);
      db_query("UPDATE fa_course_participant SET uid = :uid, invite_joined = :joined "
              ."WHERE invite_reg_code = :reg_code", array(':uid' => $account->uid, ':joined' => REQUEST_TIME, ':reg_code' => $participant->invite_reg_code));
      $update_roles = array_diff_key(_fa_course_get_new_roles($participant), $account->roles);
    }elseif($status == INVITE_REUSABLE){
      $master_participant = fa_course_participant_load($reg_code);

      // Get the number of new roles that the user does not yet have
      $new_roles = _fa_course_get_new_roles($master_participant);
      $update_roles = array_diff_key($new_roles, $account->roles);

      // Only use reuse and make new participant if new roles are added, i.e. this is not a second login with the same link
      if (!empty($update_roles)){
        db_query("UPDATE {fa_course_participant} SET invite_reuse = invite_reuse - 1 WHERE facpid = :facpid", array(':facpid' => $master_participant->facpid));
        $participant = new stdClass();
        $participant->facid = $master_participant->facid;
        $participant->email = $account->mail;
        drupal_write_record('fa_course_participant', $participant);
      }
    }

    // Process roles
    if (!empty($update_roles)){
      // Save new roles
      foreach ($update_roles as $new_role) {
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
