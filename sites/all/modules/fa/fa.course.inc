<?php
/**
* @file
* Filosofian Akatemia course specific parts.
*/

/**
 * Course Action Constants
 */
define('COURSE_SEND', 0);
define('COURSE_ADD', 1);
define('COURSE_FIX', 2);
define('COURSE_REMIND', 3);

function fa_course_form($form, &$form_state) {

  // Radio button for new/fix/remind
  $actions = array(COURSE_SEND => t('Send Preliminary Assignments'),
                   COURSE_ADD => t('Add Participants to an Existing Class'),
                   COURSE_FIX => t('Fix Invalid Email Address'),
                   COURSE_REMIND => t('Send Reminders'));

  $form['fa_course_action'] = array(
      '#type' => 'radios',
      '#title' => t('Choose Course Action'),
      '#default_value' => COURSE_SEND,
      '#options' => $actions,
  );

  // Prepare role options
  $roles = user_roles(TRUE);
  $future_class_options = array();
  $all_class_options = array();
  foreach($roles as $rid => $role){
    $role_array = explode('#', $role);
    if (count($role_array) == 3){
      // Organization
      $organization_options[$rid] = $role_array[1];
    }else if (count($role_array) == 4){
      // Product
      $product_options[$rid] = $role_array[2];
    }else if (count($role_array) > 4){
      // Class
      $all_class_options[$rid] = $role;
      // Set future class list only if date of first class is today or after
      if (date('Y-m-d') <= $role_array[3])
        $future_class_options[$rid] = $role;
    }
  }
  // Not listed options
  $organization_options[0] = t('--not listed--');
  $product_options[0] = t('--not listed--');

  // Organization select
  $form['fa_course_organization_select'] = array(
      '#type'           => 'select',
      '#multiple'       => FALSE,
      '#title'          => t('Select Organization'),
      '#options'        => $organization_options,
      '#description'    => t('Select the organization/company where the class will be kept.'),
      '#states' => array(
          'visible' => array(   // action to take.
              ':input[name=fa_course_action]' => array('value' => COURSE_SEND),
          ),
          'required' => array(
              ':input[name=fa_course_action]' => array('value' => COURSE_SEND),
          ),
      ),
  );
  $form['fa_course_organization'] = array(
      '#type'           => 'textfield',
      '#title'          => t('New Organization'),
      '#description'    => t('Type the name of the new organization here.'),
      '#states' => array(
          'visible' => array(   // action to take.
              ':input[name=fa_course_action]' => array('value' => COURSE_SEND),
              ':input[name=fa_course_organization_select]' => array('value' => 0),
          ),
          'required' => array(
              ':input[name=fa_course_action]' => array('value' => COURSE_SEND),
              ':input[name=fa_course_organization_select]' => array('value' => 0),
          ),
      ),
  );

  // Product select
  $form['fa_course_product_select'] = array(
      '#type'           => 'select',
      '#multiple'       => FALSE,
      '#title'          => t('Select Product'),
      '#options'        => $product_options,
      '#description'    => t('Select the product.'),
      '#states' => array(
          'visible' => array(   // action to take.
              ':input[name=fa_course_action]' => array('value' => COURSE_SEND),
          ),
          'required' => array(
              ':input[name=fa_course_action]' => array('value' => COURSE_SEND),
          ),
      ),
  );
  $form['fa_course_product'] = array(
      '#type'           => 'textfield',
      '#title'          => t('Custom product'),
      '#description'    => t('Type the name of the custom product here.'),
      '#states' => array(
          'visible' => array(   // action to take.
              ':input[name=fa_course_action]' => array('value' => COURSE_SEND),
              ':input[name=fa_course_product_select]' => array('value' => 0),
          ),
          'required' => array(
              ':input[name=fa_course_action]' => array('value' => COURSE_SEND),
              ':input[name=fa_course_product_select]' => array('value' => 0),
          ),
      ),
  );

  // Date of course
  $form['fa_course_date'] = array(
      '#type'           => 'date',
      '#title'          => t('Date for the first session of the course'),
      '#description'    => t('Select the date for the course'),
      '#states' => array(
          'visible' => array(   // action to take.
              ':input[name=fa_course_action]' => array('value' => COURSE_SEND),
          ),
          'required' => array(
              ':input[name=fa_course_action]' => array('value' => COURSE_SEND),
          ),
      ),
  );

  // Appendix
  $form['fa_course_appendix'] = array(
      '#type'           => 'textfield',
      '#title'          => t('Optional appendix'),
      '#required'       => FALSE,
      '#description'    => t('Optionally type an appendix for the course, such as group number if there are multiple classes on the same day.'),
      '#states' => array(
          'visible' => array(   // action to take.
              ':input[name=fa_course_action]' => array('value' => COURSE_SEND),
          ),
      ),
  );
  // Remind class selector
  $form['fa_remind_class_select'] = array(
      '#type'           => 'select',
      '#multiple'       => FALSE,
      '#title'          => t('Select class to remind'),
      '#description'    => t('Reminder will be sent to only those recipients who have not yet registered.'),
      '#options'        => $future_class_options,
      '#states' => array(
          'visible' => array(   // action to take.
              ':input[name=fa_course_action]' => array('value' => COURSE_REMIND),
          ),
          'required' => array(
              ':input[name=fa_course_action]' => array('value' => COURSE_REMIND),
          ),
      ),
  );

  // Add class selector
  $form['fa_add_class_select'] = array(
      '#type'           => 'select',
      '#multiple'       => FALSE,
      '#title'          => t('Select class to add participants to'),
      '#description'    => t('The preliminary assignment for the selected class will be emailed to the given participants.'),
      '#options'        => $all_class_options,
      '#states' => array(
          'visible' => array(   // action to take.
              ':input[name=fa_course_action]' => array('value' => COURSE_ADD),
          ),
          'required' => array(
              ':input[name=fa_course_action]' => array('value' => COURSE_ADD),
          ),
      ),
  );

  // Add the invite page form here and hide unneeded fields
  $form += invite_page_form(INVITE_UNLIMITED);
  $form['invite_roles']['#access'] = FALSE;
  $form['submit']['#access'] = FALSE;
  $form['submit_show']['#access'] = FALSE;

  unset($form['email']['#required']);
  $form['email']['#states'] = array(
          'visible' => array(   // action to take.
              ':input[name=fa_course_action]' => array('value' => COURSE_SEND),
          ),
          'required' => array(
              ':input[name=fa_course_action]' => array('value' => COURSE_SEND),
          ),
      );
  unset($form['subject']['#required']);
  $form['subject']['#states'] = array(
          'visible' => array(   // action to take.
              ':input[name=fa_course_action]' => array('value' => COURSE_SEND),
          ),
          'required' => array(
              ':input[name=fa_course_action]' => array('value' => COURSE_SEND),
          ),
      );
  $form['message']['#states'] = array(
          'visible' => array(   // action to take.
              ':input[name=fa_course_action]' => array('value' => COURSE_SEND),
                        ),
          'required' => array(
              ':input[name=fa_course_action]' => array('value' => COURSE_SEND),
          ),
      );
    $form['fa_reuse'] = array(
      '#type'           => 'textfield',
      '#title'          => t('Number of times the sent invite link can be reused. Default is zero. NOTE: If this is set to a value higher than 0, you can only give only one recipient as the person who will resend the link!'),
      '#states' => array(
          'visible' => array(   // action to take.
              ':input[name=fa_course_action]' => array('value' => COURSE_SEND),
          ),
      ),
  );

  // Add fields for adding people to classes
  $form['fa_additional_mail'] = array(
      '#type'           => 'textarea',
      '#title'          => t('Email addresses to send preliminary assignment for the class. One e-mail address per line or separated by comma.'),
      '#states' => array(
          'visible' => array(   // action to take.
              ':input[name=fa_course_action]' => array('value' => COURSE_ADD),
          ),
          'required' => array(
              ':input[name=fa_course_action]' => array('value' => COURSE_ADD),
          ),
      ),
  );

  // Add fields for fixing invalid email addresses
  $form['fa_invalid_mail'] = array(
      '#type'           => 'textfield',
      '#title'          => t('Invalid email address'),
      '#states' => array(
          'visible' => array(   // action to take.
              ':input[name=fa_course_action]' => array('value' => COURSE_FIX),
          ),
          'required' => array(
              ':input[name=fa_course_action]' => array('value' => COURSE_FIX),
          ),
      ),
  );
  $form['fa_correct_mail'] = array(
      '#type'           => 'textfield',
      '#title'          => t('Correct email address'),
      '#states' => array(
          'visible' => array(   // action to take.
              ':input[name=fa_course_action]' => array('value' => COURSE_FIX),
          ),
          'required' => array(
              ':input[name=fa_course_action]' => array('value' => COURSE_FIX),
          ),
      ),
  );

  // Summary
  $form['fa_course_summary'] = array(
      '#type'           => 'textfield',
      '#title'          => t('Summary addresses'),
      '#default_value'  => 'info@filosofianakatemia.fi, ' . $form['from']['#markup'],
      '#required'       => TRUE,
      '#description'    => t('List of addresses where to send a summary of sent/fixed invites, separated by comma.'),
  );

  // Submit
  $form['fa_course_submit'] = array(
      '#type' => 'submit',
      '#value' => t('Send invites'),
  );

  return $form;
}

function fa_course_form_validate($form, &$form_state) {

  if ($form_state['values']['fa_course_action'] == COURSE_SEND){
    $organization_role = _fa_course_get_selected_role($form_state, 'organization');
    $product_role = _fa_course_get_selected_role($form_state, 'product');
    $class_role = _fa_course_get_class_role($form_state, $organization_role, $product_role);
    // Check that a role with given class name does not yet exist
    $roles = user_roles(TRUE);

    foreach($roles as $role_name){
      if (!isset($organization_role->rid) && $role_name == $organization_role->name){
        form_set_error('fa_course_organization', t('Role name with given organization already exists'));
        return;
      }
      if (!isset($product_role->rid) && $role_name == $product_role->name){
        form_set_error('fa_course_product', t('Role name with given product already exists'));
        return;
      }
      if ($role_name == $class_role->name){
        form_set_error('fa_course_appendix', t('Role name with given organization/product/date already exists, set appropriate appendix'));
        return;
      }
    }

    // Validate given email addresses
    $user_emails = _invite_get_emails($form_state['values']['email']);
    if (_fa_course_validate_emails('email') == FALSE){
      return;
    }
    if (!empty($form_state['values']['fa_reuse'])){
      if (!is_numeric($form_state['values']['fa_reuse']) ){
        form_set_error('fa_reuse', t('Reuse value must be number'));
        return;
      }
      $form_state['values']['reuse_count'] = intval($form_state['values']['fa_reuse']);
      if ($form_state['values']['reuse_count'] < 0){
        form_set_error('fa_reuse', t('Reuse value must be a positive integer'));
        return;

      }
      if (count($user_emails)!=1){
        form_set_error('email', t('You must give exactly one email when sending a reusable invite'));
        return;
      }
    }
  }else if ($form_state['values']['fa_course_action'] == COURSE_ADD){
    if (empty($form_state['values']['fa_add_class_select'])){
      form_set_error('fa_add_class_select', t('There is no class to add participants to.'));
      return;
    }

    $additional_emails = _invite_get_emails($form_state['values']['fa_additional_mail']);
    if (_fa_course_validate_emails('fa_additional_mail')){
      if (count($additional_emails) > 0){
        // First check that none of the given emails already are in the class
        $query = db_select('invite', 'i')
          ->fields('i')
          ->condition('canceled', 0);
        $query->leftJoin('invite_roles', 'ir', 'ir.iid = i.iid');
        $query->condition('ir.rid', $form_state['values']['fa_add_class_select']);
        $query->condition('email', $additional_emails, 'IN');
        $example_invite = $query->execute()->fetchObject();
        if ($example_invite){
          form_set_error('fa_additional_mail', t('Some of the given emails are already in the class.'));
          return;
        }

        // Need to send some more invites
        // Select one invite from the group, prefer resent = 0
        $query = db_select('invite', 'i')
              ->fields('i')
              ->condition('canceled', 0);
        $query->leftJoin('invite_roles', 'ir', 'ir.iid = i.iid');
        $query->condition('ir.rid', $form_state['values']['fa_add_class_select']);
        $query->orderBy('resent', 'ASC');
        $example_invite = $query->execute()->fetchObject();

        if ($example_invite) {
          $example_invite->data = unserialize($example_invite->data);
          $form_state['values']['additional_email_class'] = $form_state['values']['fa_add_class_select'];
          $form_state['values']['additional_emails'] = $additional_emails;
          $form_state['values']['additional_invite_example'] = $example_invite;
        }else{
          form_set_error('fa_add_class_select', t('Could not find any existing non-cancelled emails for that class'));
          return;
        }
      }else{
        form_set_error('fa_additional_mail', t('No additional mails given.'));
        return;
      }
    }else{
      return;
    }
  }else if ($form_state['values']['fa_course_action'] == COURSE_FIX){
    $invalid_emails = _invite_get_emails($form_state['values']['fa_invalid_mail']);
    if (_fa_course_validate_emails('fa_invalid_mail')){
      if (count($invalid_emails) != 1){
        form_set_error('fa_invalid_mail', t('Exactly one invalid email can be fixed at a time'));
        return;
      }

      $invite = db_select('invite', 'i')
        ->fields('i')
        ->condition('email', $invalid_emails[0])
        ->condition('canceled', 0)
        ->execute()
        ->fetchObject();

      if ($invite) {
        $invite->data = unserialize($invite->data);
        // Store the code for later use to avoid duplicating query
        $form_state['values']['invalid_email'] = $invalid_emails[0];
        $form_state['values']['invite'] = $invite;
      }else{
        form_set_error('fa_invalid_mail', t('Could not find an invite with the given invalid email'));
        return;
      }
    }else{
      return;
    }

    $correct_emails = _invite_get_emails($form_state['values']['fa_correct_mail']);
    if (_fa_course_validate_emails('fa_correct_mail')){
      if (count($correct_emails) != 1){
        form_set_error('fa_correct_mail', t('Exactly one correct email can be given at a time'));
        return;
      }
      $form_state['values']['correct_email'] = $correct_emails[0];
    }else{
      return;
    }

    // Check that the fixed and the correct are not the same address
    if ($form_state['values']['correct_email'] == $form_state['values']['invalid_email']){
      form_set_error('fa_correct_mail', t('Give a different address to the correct email field!'));
      return;
    }

  }else if ($form_state['values']['fa_course_action'] == COURSE_REMIND){
    if (empty($form_state['values']['fa_remind_class_select'])){
      form_set_error('fa_remind_class_select', t('There is no pending class to remind.'));
      return;
    }
    $query = db_select('invite', 'i')
               ->fields('i')
               ->condition('canceled', 0)
               ->condition('joined', 0);
    $query->leftJoin('invite_roles', 'ir', 'ir.iid = i.iid');
    $query->condition('ir.rid', $form_state['values']['fa_remind_class_select']);
    $remind_invites = $query->execute();

    if ($remind_invites->rowCount() == 0){
      form_set_error('fa_remind_class_select', t('No one to remind in the selected class'));
      return;
    }

    $form_state['values']['remind_invites'] = array();
    foreach($remind_invites as $remind_invite){
      $remind_invite->data = unserialize($remind_invite->data);
      array_push($form_state['values']['remind_invites'], $remind_invite);
    }
  }

  // Check summary emails
  $summary_emails = _invite_get_emails($form_state['values']['fa_course_summary']);
  if (_fa_course_validate_emails('fa_course_summary') == FALSE){
    return;
  }
}

function fa_course_form_submit($form, &$form_state) {

  if ($form_state['values']['fa_course_action'] == COURSE_SEND){
    // Organization role
    $organization_role = _fa_course_get_selected_role($form_state, 'organization');
    if (!isset($organization_role->rid)){
      // Organization role needs to be saved
      user_role_save($organization_role);
    }

    // Product role
    $product_role = _fa_course_get_selected_role($form_state, 'product');
    if (!isset($product_role->rid)){
      // Product role needs to be saved
      user_role_save($product_role);
    }

    // Class role
    $class_role = _fa_course_get_class_role($form_state, $organization_role, $product_role);
    user_role_save($class_role);

    // Set all three roles to the invite
    $form_state['values']['invite_roles'] = array(
        $organization_role->rid => $organization_role->name,
        $product_role->rid => $product_role->name,
        $class_role->rid => $class_role->name);

    // Search for a preliminary assignment
    $preliminary_assignment_url = db_query("SELECT ua.alias from node n INNER JOIN url_alias ua ON ua.source = CONCAT('node/', n.nid) "
                                         . "WHERE n.type = 'preliminary_assignment' AND n.title LIKE :title",
                                          array(':title' => substr($product_role->name, 2, (strlen($product_role->name)-3)) . '%')
                                          )->fetchField();
    if (!empty($preliminary_assignment_url)){
      // Change the [invite:join-link] to contain the destination
      $form_state['values']['message'] = str_replace("[invite:join-link]",
          "[invite:join-link]?destination=" . $preliminary_assignment_url, $form_state['values']['message']);
    }

    // Send invites by validating and submitting the form
    invite_form_validate($form, $form_state);
    invite_form_submit($form, $form_state);
  }else if ($form_state['values']['fa_course_action'] == COURSE_ADD){
    // Create new invites, invite data from an example invite
    foreach ($form_state['values']['additional_emails'] as $additional_email) {
      $additional_invite = clone $form_state['values']['additional_invite_example'];
      unset($additional_invite->iid);
      $additional_invite->reg_code = invite_generate_code();
      $additional_invite->data['message'] = str_replace(
          $form_state['values']['additional_invite_example']->reg_code,
          $additional_invite->reg_code,
          $additional_invite->data['message']);
      $additional_invite->email = $additional_email;
      $additional_invite->joined = 0;
      $additional_invite->resent = 0;
      $additional_invite->is_new = TRUE;

      if (!isset($additional_invite_subject) && !isset($additional_invite_message)){
        $additional_invite_subject = $additional_invite->data['subject'];
        $additional_invite_message = str_replace($additional_invite->reg_code, '[invite:reg-code]', $additional_invite->data['message']);
      }

      if (invite_send($additional_invite) == FALSE){
        form_set_error('fa_course_action', 'Error while sending email to additional participants');
        return;
      }
      update_role_mappings($additional_invite, _get_roles($form_state['values']['additional_invite_example']));
    }
  }else if ($form_state['values']['fa_course_action'] == COURSE_FIX){
    $form_state['values']['invite']->email = $form_state['values']['correct_email'];
    // Store unserialized data for reference in summary
    $form_state['values']['invite_data'] = $form_state['values']['invite']->data;
    if (invite_send($form_state['values']['invite']) == FALSE){
      form_set_error('fa_correct_mail', 'Error while sending email to correct address');
      return;
    }
  }else if ($form_state['values']['fa_course_action'] == COURSE_REMIND){
    foreach ($form_state['values']['remind_invites'] as $remind_invite){
      $remind_prefix = t('Reminder:');
      if (substr($remind_invite->data['subject'], 0, strlen($remind_prefix)) != $remind_prefix){
        $remind_invite->data['subject'] = $remind_prefix . " " . $remind_invite->data['subject'];
      }
      if (invite_send($remind_invite) == FALSE){
        form_set_error('fa_course_action', 'Error while sending reminder to email');
        return;
      }
    }
  }

  // Send summaries if succeeded
  if (form_get_errors()) return;

  // Get email addresses
  $summary_emails = _invite_get_emails($form_state['values']['fa_course_summary']);

  // Create summary message
  if ($form_state['values']['fa_course_action'] == COURSE_SEND){
    $summary = array(
        'subject' => t("SUMMARY: ") . trim($form_state['values']['subject']),
    );
    if (isset($form_state['values']['reuse_count'])){
      $summary['message'] = t("A reusable invitation using the following message template was sent to this recipient:" . "\n\n");
    }else{
      $summary['message'] = t("An invitation using the following message template was sent to these recipients:" . "\n\n");
    }
    $user_emails = _invite_get_emails($form_state['values']['email']);
    foreach ($user_emails as $user_email) {
      $summary['message'] .= $user_email . "\n";
    }
    $summary['message'] .= "\n**********\n\n";
    $summary['message'] .= trim($form_state['values']['subject']) . "\n\n";
    $summary['message'] .= $form_state['values']['message'];
  }else if ($form_state['values']['fa_course_action'] == COURSE_ADD){
    $summary = array(
        'subject' => t("ADDED: ") . $additional_invite_subject,
        'message' => 'An invitation like following was sent to these additional recipients:' . "\n\n");
    foreach ($form_state['values']['additional_emails'] as $additional_email) {
      $summary['message'] .= $additional_email . "\n";
    }
    $summary['message'] .= "\n**********\n\n";
    $summary['message'] .= $additional_invite_subject . "\n\n";
    $summary['message'] .= $additional_invite_message;
    drupal_set_message('Sent ' . count($form_state['values']['additional_emails']) . ' additional invites.', 'status');
  }else if ($form_state['values']['fa_course_action'] == COURSE_FIX){
    $summary = array(
        'subject' => t("FIXED: ") . trim($form_state['values']['invite_data']['subject']),
        'message' => t("The following message was sent to the fixed email address:" . "\n\n"),
    );
    $summary['message'] .= $form_state['values']['invalid_email'] . ' => ' . $form_state['values']['correct_email'] . "\n";
    $summary['message'] .= "\n**********\n\n";
    $summary['message'] .= trim($form_state['values']['invite_data']['subject']) . "\n\n";
    $summary['message'] .= $form_state['values']['invite_data']['message'];
    drupal_set_message(t('Email address fixed and summary sent'), 'status');
  }else if ($form_state['values']['fa_course_action'] == COURSE_REMIND){
    $summary = array(
        'subject' => t("REMINDED:"),
        'message' => t("Message like the following was sent to these recipients:" . "\n\n"),
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
    drupal_set_message('Sent ' . count($form_state['values']['remind_invites']) . ' reminders.', 'status');
  }

  $from = variable_get('invite_manual_from', NULL);
  global $language;
  foreach ($summary_emails as $summary_email) {
    // Send the summary
    $result = drupal_mail('fa', 'fa', $summary_email, $language, $summary, $from, TRUE);
  }
}

/**
 * Implements hook_mail().
 */
function fa_mail($key, &$message, $summary) {
  global $user;
  if (isset($summary['subject'])) $message['subject'] = $summary['subject'];
  if (isset($summary['message'])) $message['body'][] = $summary['message'];
}

function _fa_course_get_class_role($form_state, $organization_role, $product_role){
  // Create class role based on organization, product and time
  $class_role = new stdClass();
  $class_date = $form_state['values']['fa_course_date'];
  $class_role->name = $organization_role->name . substr($product_role->name, 2, (strlen($product_role->name)-2))
                      . $class_date['year'] . '-' . str_pad($class_date['month'], 2, '0', STR_PAD_LEFT)
                      . '-' . str_pad($class_date['day'], 2, '0', STR_PAD_LEFT). '#';
  if (!empty ($form_state['values']['fa_course_appendix'])){
    $class_role->name .= $form_state['values']['fa_course_appendix'] . '#';
  }
  return $class_role;
}

function _fa_course_get_selected_role($form_state, $role_type){
  $selected_role = new stdClass();
  if (!empty($form_state['values']['fa_course_' . $role_type])){
    // Role needs to be created
    $selected_role->name = '';
    if ($role_type == 'product'){
      $selected_role->name .= '#';
    }
    $selected_role->name .= '#' . $form_state['values']['fa_course_' . $role_type] . '#';
  }else{
    $roles = user_roles(TRUE);
    $selected_role->rid = $form_state['values']['fa_course_' . $role_type . '_select'];
    $selected_role->name = $roles[$selected_role->rid];
  }
  return $selected_role;
}

function _fa_course_validate_emails($form_field){
  if (isset($_SESSION['invite_failed_emails'])){
    $failed_emails = (array) unserialize($_SESSION['invite_failed_emails']);
    $error_message = t('Invalid email addresses:');
    foreach ($failed_emails as $failed_email) {
      $error_message .= ' ' . $failed_email;
    }
    form_set_error($form_field, $error_message);
    unset($_SESSION['invite_failed_emails']);
    return FALSE;
  }
  return TRUE;
}

function _get_roles($invite){
  // Gets all roles based on given invite
  $query = db_select('invite', 'i')
      ->fields('r', array('rid', 'name'))
      ->condition('i.iid', $invite->iid);
  $query->leftJoin('invite_roles', 'ir', 'ir.iid = i.iid');
  $query->leftJoin('role', 'r', 'ir.rid = r.rid');
  $result = $query->execute()->fetchAll();

  $invite_roles = array();
  foreach($result as $role_row){
    $invite_roles[$role_row->rid] = $role_row->name;
  }
  return $invite_roles;
}
