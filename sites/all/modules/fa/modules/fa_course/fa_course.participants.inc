<?php

/**
 * @file
 * Course participant editing functions for Filosofian Akatemia Course module.
 */

define('EXCEL', 1);
define('NAMES', 2);

/**
 * Return a list of all course participants.
 */
function fa_course_participants($facid) {
  $header = array(
    array('data' => t('E-mail'), 'field' => 'p.email', 'sort' => 'asc'),
    array('data' => t('First Name'), 'field' => 'p.first_name'),
    array('data' => t('Last Name'), 'field' => 'p.last_name'),
    t('Joined'),
    t('Invite reg code'),
    t('Invite reuse'),
    t('Actions'),
  );

  $result = db_select('fa_course_participant', 'p')
    ->fields('p', array('facid', 'facpid', 'email', 'first_name', 'last_name', 'invite_joined', 'invite_reg_code', 'invite_reuse', 'uid'))
    ->extend('TableSort')
    ->extend('PagerDefault');
  $result->condition('p.facid', $facid);
  $result->distinct();
  $result->groupBy('p.facid');
  $result->groupBy('p.facpid');

  $result->orderByHeader($header);
  $result = $result->execute()->fetchAll();

  $rows = array();
  foreach ($result as $row) {
    $cells = array();
    $cells[] = isset($row->uid) ? l($row->email, "user/" . $row->uid) : $row->email;
    $cells[] = isset($row->first_name) ? $row->first_name : t('<i>Not set</i>');
    $cells[] = isset($row->last_name) ? $row->last_name : t('<i>Not set</i>');
    $cells[] = $row->invite_joined;
    $cells[] = $row->invite_reg_code;
    $cells[] = $row->invite_reuse;
	$cells[] = (user_access('administer courses'))
						? l(t('Edit'), "admin/people/courses/participants/$facid/edit/$row->facpid") . '<br/>' .
						  l(t('Delete'), "admin/people/courses/participants/$facid/delete/$row->facpid")
						: "";
    $rows[] = $cells;
  }

  $links = '<p><strong>' . _fa_course_get_course_name($facid) . '</strong><br/>'
             . l(t("Back"), "admin/people/courses/view/$facid") . '<br/>'
             . l(t("Import participants"), "admin/people/courses/participants/$facid/import") . '<br/>'
             . l(t("Add new participant"), "admin/people/courses/participants/$facid/edit/new") . '</p>';
  $output = render($links);

  $output .= theme('table', array('header' => $header, 'rows' => $rows));
  if (!$rows) {
    $output .= t('No participants found.');
  }
  else {
    $output .= theme('pager');
  }

  return $output;
}

function fa_course_participants_edit($facid, $facpid){
  $links = '<p><strong>' . _fa_course_get_course_name($facid) . '</strong><br/>'
            . l(t("Back to participants"), "admin/people/courses/participants/$facid") . '</p>';
  $output = render($links);

  $participant = fa_course_get_participant($facid, $facpid);
  $edit_form = drupal_get_form('fa_course_participants_edit_form', $participant);
  $output .= render($edit_form);
  return $output;
}

function fa_course_get_participant($facid, $facpid){
  if ($facpid != 'new'){
    $participant = db_select('fa_course_participant', 'p')
    ->fields('p')
    ->condition('p.facpid', $facpid)
    ->condition('p.facid', $facid)
    ->execute()->fetchObject();

  }else {
    $participant = (object) array(
        'facid'  => $facid,
    );
  }
  return $participant;
}

/**
 * Form for creating/editing a course participant.
 */
function fa_course_participants_edit_form($form, &$form_state, $participant) {
  global $user;

  $form['participant'] = array(
      '#type' => 'fieldset',
      '#title' => t('Edit participant'),
  );
  $form['participant']['id'] = array(
      '#type' => 'textfield',
      '#title' => t('Participant Id'),
      '#disabled' => TRUE,
      '#value' => isset($participant->facpid) ? $participant->facpid : 'N/A',
  );
  $form['participant']['facid'] = array(
      '#type' => 'textfield',
      '#title' => t('Course Id'),
      '#disabled' => TRUE,
      '#value' => $participant->facid,
  );

  $form['participant']['first_name'] = array(
      '#type'           => 'textfield',
      '#title'          => t('First Name'),
      '#default_value'  => isset($participant->first_name) ? $participant->first_name : NULL,
  );

  $form['participant']['last_name'] = array(
      '#type'           => 'textfield',
      '#title'          => t('Last Name'),
      '#default_value'  => isset($participant->last_name) ? $participant->last_name : NULL,
  );

  $form['participant']['email'] = array(
      '#type'           => 'textfield',
      '#title'          => t('Email address'),
      '#required'       => TRUE,
      '#default_value'  => isset($participant->email) ? $participant->email : NULL,
  );

  $form['participant']['invite_reuse'] = array(
      '#type'           => 'textfield',
      '#title'          => t('Number of times the invite can be reused'),
      '#description'    => t('Set this only if someone else wants to send invitations to the participants on your behalf.'),
      '#required'       => FALSE,
      '#size'           => 4,
      '#maxlength'      => 4,
      '#default_value'  => isset($participant->invite_reuse) ? $participant->invite_reuse : NULL,
  );

  $form['participant']['submit'] = array(
      '#type' => 'submit',
      '#value' => t('Save'),
  );

  $form_state['participant'] = $participant;
  return $form;
}

function fa_course_participants_edit_form_validate($form, &$form_state) {
  // Check email
  $email_field = _fa_course_get_emails($form_state['values']['email']);
  if (_fa_course_validate_one_email($email_field, 'email') == FALSE){
    return;
  }

  if (!isset($form_state['participant']->facpid) || ($form_state['participant']->email != $email_field['valid'][0])){
    // Check that email is unique for this course
    if (db_select('fa_course_participant', 'p')
            ->fields('p')
            ->condition('p.email', $email_field['valid'][0])
            ->condition('p.facid', $form_state['participant']->facid)
            ->execute()->fetchObject() != FALSE){
      form_set_error('email', t('Course already has a participant with given email address'));
    }
  }

  if (!empty($form_state['values']['invite_reuse'])){
    // Check that there are no other participants
    if (!isset($form_state['participant']->facpid) &&
      (db_select('fa_course_participant', 'p')
            ->fields('p')
            ->condition('p.facid', $form_state['participant']->facid)
            ->isNotNull('p.invite_reg_code')
            ->execute()->fetchObject() != FALSE)){
      form_set_error('invite_reuse', t('Course already has an added participant with a register code, reuse invite has to be the only added participant.'));
    }
  }else{
    if (!isset($form_state['participant']->facpid) &&
      (db_select('fa_course_participant', 'p')
            ->fields('p')
            ->condition('p.facid', $form_state['participant']->facid)
            ->isNotNull('p.invite_reuse')
            ->execute()->fetchObject() != FALSE)){
      form_set_error('email', t('Course already has a participant who has a reusable invite.'));
    }
  }
}

function fa_course_participants_edit_form_submit($form, &$form_state) {
  $participant = $form_state['participant'];
  if (!empty($form_state['values']['first_name'])) $participant->first_name = $form_state['values']['first_name'];
  if (!empty($form_state['values']['last_name'])) $participant->last_name = $form_state['values']['last_name'];
  $participant->invite_reuse = !empty($form_state['values']['invite_reuse']) ? $form_state['values']['invite_reuse'] : NULL;
  if (!isset($participant->invite_reg_code)){
    // Generate reg code
    $participant->invite_reg_code = fa_course_generate_reg_code();
  }

  module_load_include('inc', 'fa_course', 'fa_course.email');
  $new_participant = FALSE;
  if (isset($participant->email) && $participant->email != $form_state['values']['email']){
    $old_email = $participant->email;
    $participant->email = $form_state['values']['email'];
    $number_of_emails = fa_course_fix_email($participant, $old_email);
    if ($number_of_emails == -1){
      drupal_set_message(t('Could not send participant email to new address, save failed.'), 'error');
    }elseif ($number_of_emails == 1){
      drupal_set_message(t('One email sent to new address.'));
    }
  }elseif(!isset($participant->email)){
    $new_participant = TRUE;
    $participant->email = trim($form_state['values']['email']);
    drupal_write_record('fa_course_participant', $participant);
    $number_of_emails = fa_course_add_email($participant);
    if ($number_of_emails == -1){
      drupal_set_message(t('Could not send new participant email, save failed.'), 'error');
    }elseif ($number_of_emails == 1){
      drupal_set_message(t('One email sent to new participant.'));
    }
  }

  if (!$new_participant)
    drupal_write_record('fa_course_participant', $participant, 'facpid');

  drupal_set_message(t('Participant @facpid saved.', array('@facpid' => $participant->facpid)));
  $form_state['redirect'] = "admin/people/courses/participants/" . $participant->facid;
}

function fa_course_participants_delete($facid, $facpid){
  $links = '<p>' . l(t("Back to participants"), "admin/people/courses/participants/$facid") . '</p>';
  $output = render($links);
  $participant = fa_course_get_participant($facid, $facpid);
  $delete_form = drupal_get_form('fa_course_participants_delete_form', $participant);
  $output .= render($delete_form);
  return $output;

}

function fa_course_participants_delete_form($form, &$form_state, $participant) {
  $form['participant'] = array(
    '#type' => 'value',
    '#value' => $participant,
  );

  return confirm_form(
    $form,
    t('Are you sure you want to delete the participant %email?', array('%email' => $participant->email)),
    "admin/people/courses/participants/" . $participant->facid,
    t('This action cannot be undone.'),
    t('Delete'),
    t('Cancel')
  );
}

function fa_course_participants_delete_form_submit($form, &$form_state) {
  $participant = $form_state['values']['participant'];

	db_delete('fa_course_participant')
	  ->condition('facpid', $participant->facpid)
	  ->execute();

  drupal_set_message(t('Participant with email %email has been deleted.', array('%email' => $participant->email)));
  $form_state['redirect'] = "admin/people/courses/participants/" . $participant->facid;
}


function fa_course_participants_import($facid){
  $links = '<p><strong>' . _fa_course_get_course_name($facid) . '</strong><br/>'
            . l(t("Back to participants"), "admin/people/courses/participants/$facid") . '</p>';
  $output = render($links);

  $import_form = drupal_get_form('fa_course_participants_import_form', $facid);
  $output .= render($import_form);
  return $output;
}


/**
 * Form for importing course participants.
 */
function fa_course_participants_import_form($form, &$form_state, $facid) {
  global $user;

  $form['import_participants'] = array(
      '#type' => 'fieldset',
      '#title' => t('Import participants'),
  );
  $form['import_participants']['facid'] = array(
      '#type' => 'textfield',
      '#title' => t('Course Id'),
      '#disabled' => TRUE,
      '#value' => $facid,
  );

  $form['import_participants']['action'] = array(
      '#type'           => 'radios',
      '#title'          => t('Import action'),
      '#required'       => TRUE,
      '#default_value'  => EXCEL,
      '#options'        => array(EXCEL => t('Paste email, first name, last name from Excel'),
                                 NAMES => t('First name, last name and domain')),
  );

  $form['import_participants']['participants'] = array(
      '#type'           => 'textarea',
      '#required'       => TRUE,
      '#title'          => t('Participants to import, one per row'),
  );

  $form['import_participants']['domain'] = array(
      '#type'           => 'textfield',
      '#title'          => t('Email domain'),
      '#states' => array(
          'visible' => array(
              ':input[name=action]' => array('value' => NAMES),
              ),
      ),
  );

  $form['import_participants']['submit'] = array(
      '#type' => 'submit',
      '#value' => t('Import'),
  );

  $form_state['facid'] = $facid;
  return $form;
}

function fa_course_participants_import_form_validate($form, &$form_state){
  if ($form_state['values']['action'] == NAMES && empty($form_state['values']['domain'])){
    form_set_error('domain', t('Domain must be given if importing names'));
    return;
  }
  $participant_rows = preg_split( '/\r\n|\r|\n/', $form_state['values']['participants'] );
  $participants = array();
  foreach($participant_rows as $participant){
    if ($participant != ''){
      if ($form_state['values']['action'] == NAMES){
        $names = explode(' ', $participant);
        if (count($names) != 2){
          form_set_error('participants', t('Names must be in the format [first_name] [last_name] ' . count($names)));
          return;
        }
        $email = _unaccent(strtolower(
                    str_replace(' ', '.', $participant) . '@' . $form_state['values']['domain']));

        if (!isset($form_state['values']['emails'])) $form_state['values']['emails'] = $email;
        else $form_state['values']['emails'] .= ', ' . $email;
        array_push($participants, array('email' => $email,
                                        'first_name' => $names[0],
                                        'last_name' => $names[1]));
      }else{
        $details = preg_split('/\t/', $participant);

        if (count($details) == 1){
          array_push($participants, array('email' => $details[0]));
        }elseif (count($details) == 3){
          array_push($participants, array('email' => $details[0],
                                        'first_name' => $details[1],
                                        'last_name' => $details[2]));
        }else{
          form_set_error('participants', t('Excel sheet must be in tab limited format [email] ([first_name] [last_name])'));
          return;
        }
        if (!isset($form_state['values']['emails'])) $form_state['values']['emails'] = $details[0];
        else $form_state['values']['emails'] .= ', ' . $details[0];
      }
    }
  }
  if (empty($participants)){
    form_set_error('participants', t('No participants found'));
    return;
  }

  // Validate participant emails before they are imported
  $participant_email_fields = _fa_course_get_emails($form_state['values']['emails']);
  _fa_course_validate_emails($participant_email_fields, 'emails');

  $form_state['participants'] = $participants;
}

function fa_course_participants_import_form_submit($form, &$form_state) {
  foreach($form_state['participants'] as $participant){
    $form_state['participant'] = (object) array(
                                          'facid'  => $form_state['values']['facid'],
                                      );
    $form_state['values']['email'] = $participant['email'];
    $form_state['values']['first_name'] = isset($participant['first_name']) ? $participant['first_name'] : NULL;
    $form_state['values']['last_name'] = isset($participant['last_name']) ? $participant['last_name'] : NULL;
    fa_course_participants_edit_form_validate($form, $form_state);
    fa_course_participants_edit_form_submit($form, $form_state);
  }
  $form_state['redirect'] = "admin/people/courses/participants/" . $form_state['facid'];
}

function _unaccent($string)
{
    if (strpos($string = htmlentities($string, ENT_QUOTES, 'UTF-8'), '&') !== false)
    {
        $string = html_entity_decode(preg_replace('~&([a-z]{1,2})(?:acute|cedil|circ|grave|lig|orn|ring|slash|tilde|uml);~i', '$1', $string), ENT_QUOTES, 'UTF-8');
    }

    return $string;
}
