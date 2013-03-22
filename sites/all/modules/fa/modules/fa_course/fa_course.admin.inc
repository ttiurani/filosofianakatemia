<?php

/**
 * @file
 * Administration functions for Filosofian Akatemia Course module.
 */


/**
 * Schedule Action Constants
 */
define('SCHEDULE', 1);
define('SEND_NOW', 2);
define('DATE_FORMAT', 'd.m.Y H:i');
define('SECONDS_IN_DAY', 86400);


define('EXCEL', 1);
define('NAMES', 2);



/**
 * Return a list of all courses.
 */
function fa_course_admin_overview() {
  $header = array(
    array('data' => t('Course Id'), 'field' => 'facid'),
    array('data' => t('First session'), 'field' => 'first_session', 'sort' => 'asc'),
    array('data' => t('Organization'), 'field' => 'organization'),
    array('data' => t('Product'), 'field' => 'product'),
    array('data' => t('Lead Sales'), 'field' => 'sales'),
    array('data' => t('Lead Trainer'), 'field' => 'trainer'),
    t('City'),
    t('Course role'),
    t('CRM'),
    t('Phase'),
    t('Sessions'),
    t('Participants'),
    t('Actions'),
  );

  $result = db_select('fa_course', 'fac')
    ->fields('fac', array('facid', 'city', 'crm_link', 'trainer_handover', 'sales_handover', 'lead_sales_id', 'lead_trainer_id'))
    ->extend('TableSort')
    ->extend('PagerDefault');
  $result->innerJoin('role', 'ro', 'ro.rid = fac.organization_id');
  $result->addField('ro', 'name', 'organization');
  $result->innerJoin('role', 'rp', 'rp.rid = fac.product_id');
  $result->addField('rp', 'name', 'product');
  $result->innerJoin('users', 'us', 'us.uid = fac.lead_sales_id');
  $result->innerJoin('field_data_field_first_name', 'us_fn', 'us_fn.entity_id = us.uid');
  $result->addField('us_fn', 'field_first_name_value', 'sales_first_name');
  $result->innerJoin('field_data_field_last_name', 'us_ln', 'us_ln.entity_id = us.uid');
  $result->addField('us_ln', 'field_last_name_value', 'sales_last_name');
  $result->leftJoin('users', 'ut', 'ut.uid = fac.lead_trainer_id');
  $result->leftJoin('field_data_field_first_name', 'ut_fn', 'ut_fn.entity_id = ut.uid');
  $result->addField('ut_fn', 'field_first_name_value', 'trainer_first_name');
  $result->leftJoin('field_data_field_last_name', 'ut_ln', 'ut_ln.entity_id = ut.uid');
  $result->addField('ut_ln', 'field_last_name_value', 'trainer_last_name');
  $result->leftJoin('role', 'rc', 'rc.rid = fac.course_id');
  $result->addField('rc', 'name', 'course_role');
  $result->leftJoin('fa_course_session', 's', 's.facid = fac.facid');
  $result->addExpression('COUNT(DISTINCT s.facsid)', 'sessions');
  $result->addExpression('MIN(s.session_start_datetime)', 'first_session');
  $result->leftJoin('fa_course_participant', 'p', 'p.facid = fac.facid');
  $result->addExpression('COUNT(DISTINCT p.facpid)', 'participants');
  $result->distinct();
  $result->groupBy('fac.facid');

  $result->orderByHeader($header);
  $result = $result->execute()->fetchAll();

  $rows = array();
  foreach ($result as $row) {
    $cells = array();
    $cells[] = $row->facid;
    $cells[] = isset($row->first_session) ? format_date($row->first_session, 'custom', 'D d.m.Y H:i') : t('Not set');
    $cells[] = str_replace('#', '', $row->organization);
    $cells[] = str_replace('#', '', $row->product);
    $cells[] = l($row->sales_first_name . " " . $row->sales_last_name, "user/" . $row->lead_sales_id);
    $cells[] = isset($row->trainer_first_name) ? l($row->trainer_first_name . " " . $row->trainer_last_name, "user/" . $row->lead_trainer_id) : t('Not set');
    $cells[] = $row->city;
    $cells[] = $row->course_role == NULL ? t('Not set') : $row->course_role;
    $cells[] = l(t("View CRM"), $row->crm_link, array('attributes' => array('target'=>'_blank')));

    $phase_options = array(
        'pending' =>  array(
            'title' => t("Sales pending more information"),
            'action' => l(t("Handover to the trainer"), "admin/people/courses/trainer_handover/$row->facid")),
        'trainer' =>  array(
            'title' => t("Trainer responsibility"),
            'action' => l(t("Handover back to sales"), "admin/people/courses/sales_handover/$row->facid")),
        'finished' =>  array(
            'title' => t("Sales responsible for resale")),
    );
    $phase = $phase_options['pending'];
    if (isset($row->trainer_handover)){
      $phase = $phase_options['trainer'];
      if (isset($row->sales_handover)){
          $phase = $phase_options['finished'];
      }
    }
    $cells[] = $phase['title'] . (isset($phase['action']) && user_access('administer courses') ? " <br/> " . $phase['action'] : "");
    $cells[] = $row->sessions . "<br/>" . l(t("Details"), "admin/people/courses/sessions/$row->facid");
    $cells[] = $row->participants . "<br/>" . l(t("Details"), "admin/people/courses/participants/$row->facid");
    $cells[] = l(t('Details'), "admin/people/courses/view/$row->facid")
              . (user_access('administer courses')
                    ? " <br/> " . l(t('Edit'), "admin/people/courses/edit/$row->facid") . '<br/>'
                                . l(t('Delete'), "admin/people/courses/delete/$row->facid")
                    : "");
    $rows[] = $cells;
  }

  $output = "";
  if (user_access('administer courses')){
    $add_new = '<p>' . l(t("Add new course"), "admin/people/courses/edit/new") . '</p>';
    $output .= render($add_new);
  }

  $output .= theme('table', array('header' => $header, 'rows' => $rows));
  if (!$rows) {
    $output .= t('No courses found.');
  }
  else {
    $output .= theme('pager');
  }

  return $output;
}

function fa_course_admin_details($action, $facid){
  if ($action == 'view' && $facid == 'new'){
  	return t('Invalid action');
  }
  if ($facid != 'new'){
    $course = db_select('fa_course', 'fac')
      ->fields('fac')
      ->condition('fac.facid', $facid)
      ->execute()->fetchObject();

  }else {
    $course = (object) array(
      'created'  => REQUEST_TIME,
    );
  }

  $links = '<p>' . l(t("Back to courses"), "admin/people/courses") . '</p>';
  $output = render($links);

  $edit_form = drupal_get_form('fa_course_admin_edit_form', $course, $action);
  $output .= render($edit_form);

  return $output;
}

/**
 * Helper function to get
 */
function _fa_course_get_academians(){
  // Use $query for readability
  $query = 'SELECT DISTINCT u.uid, u.name
              FROM {users} u
              INNER JOIN {users_roles} ur ON u.uid = ur.uid
              WHERE ur.rid = 4';
  $result = db_query($query, array(':rids' => array(1,2,3)));

  $academians = array();
  foreach ($result as $row) {
    $academians[$row->uid] = $row->name;
  }
  return $academians;
}


/**
 * Form for creating/editing a course.
 */
function fa_course_admin_edit_form($form, &$form_state, $course, $action) {
  global $user;

  $academians = _fa_course_get_academians();
  if (!isset($course->lead_trainer_id))
    $academians[0] = t('-- unknown --');

  // Prepare role options
  $roles = user_roles(TRUE);
  foreach($roles as $rid => $role){
    $role_array = explode('#', $role);
    if (count($role_array) == 3){
      // Organization
      $organization_options[$rid] = $role_array[1];
    }else if (count($role_array) == 4){
      // Product
      $product_options[$rid] = $role_array[2];
    }
  }
  // Not listed options
  $organization_options[0] = t('--not listed--');
  $product_options[0] = t('--not listed--');

  $form['course'] = array(
      '#type' => 'fieldset',
      '#title' => t('Edit Course'),
  );
  $form['course']['id'] = array(
      '#type' => 'textfield',
      '#title' => t('Id'),
      '#disabled' => TRUE,
      '#value' => isset($course->facid) ? $course->facid : 'N/A',
  );

  // Organization select
  $form['course']['organization_select'] = array(
      '#type'           => 'select',
      '#multiple'       => FALSE,
      '#title'          => t('Select Organization'),
      '#options'        => $organization_options,
      '#disabled'       => $action == 'view',
      '#description'    => t('Select the organization/company where the course will be kept.'),
  );
  $form['course']['organization'] = array(
      '#type'           => 'textfield',
      '#title'          => t('New Organization'),
      '#disabled'       => $action == 'view',
      '#description'    => t('Type the name of the new organization here.'),
      '#states' => array(
          'visible' => array(   // action to take.
              ':input[name=organization_select]' => array('value' => 0),
          ),
          'required' => array(
              ':input[name=organization_select]' => array('value' => 0),
          ),
      ),
  );

  // Product select
  $form['course']['product_select'] = array(
      '#type'           => 'select',
      '#multiple'       => FALSE,
      '#title'          => t('Select Product'),
      '#disabled'       => $action == 'view',
      '#options'        => $product_options,
      '#description'    => t('Select the product.'),
  );
  $form['course']['product'] = array(
      '#type'           => 'textfield',
      '#title'          => t('Custom product'),
      '#disabled'       => $action == 'view',
      '#description'    => t('Type the name of the custom product here.'),
      '#states' => array(
          'visible' => array(   // action to take.
              ':input[name=product_select]' => array('value' => 0),
          ),
          'required' => array(
              ':input[name=product_select]' => array('value' => 0),
          ),
      ),
  );

  $form['course']['city'] = array(
      '#type' => 'textfield',
      '#title' => t('City'),
      '#disabled'       => $action == 'view',
      '#default_value' => isset($course->city) ? $course->city: 'Helsinki',
      '#required' => TRUE,
  );
  $form['course']['crm_link'] = array(
      '#type' => 'textfield',
      '#title' => t('CRM Link'),
      '#disabled'       => $action == 'view',
      '#default_value' => isset($course->crm_link) ? $course->crm_link: NULL,
      '#required' => TRUE,
  );
  $form['course']['lead_sales'] = array(
      '#type'           => 'select',
      '#multiple'       => FALSE,
      '#title'          => t('Lead Sales'),
      '#disabled'       => $action == 'view',
      '#options'        => $academians,
      '#default_value'  => isset($course->lead_sales_id) ? $course->lead_sales_id :
                          (isset($academians[$user->uid]) ? $user->uid : NULL),
  );
  $form['course']['lead_trainer'] = array(
      '#type'           => 'select',
      '#multiple'       => FALSE,
      '#title'          => t('Lead Trainer'),
      '#disabled'       => $action == 'view',
      '#options'        => $academians,
      '#default_value'  => isset($course->lead_trainer_id) ? $course->lead_trainer_id : 0,
  );
  $form['course']['submit'] = array(
      '#type' => 'submit',
      '#disabled'       => $action == 'view',
      '#value' => t('Save'),
  );

  $form_state['course'] = $course;
  return $form;
}

function _fa_course_get_selected_role($form_state, $role_type){
  $selected_role = new stdClass();
  if (!empty($form_state['values']['fa_invite_' . $role_type])){
    // Role needs to be created
    $selected_role->name = '';
    if ($role_type == 'product'){
      $selected_role->name .= '#';
    }
    $selected_role->name .= '#' . $form_state['values'][$role_type] . '#';
  }else{
    $roles = user_roles(TRUE);
    $selected_role->rid = $form_state['values'][$role_type . '_select'];
    $selected_role->name = $roles[$selected_role->rid];
  }
  return $selected_role;
}

function fa_course_admin_edit_form_validate($form, &$form_state) {
  $organization_role = _fa_course_get_selected_role($form_state, 'organization');
  $product_role = _fa_course_get_selected_role($form_state, 'product');
  // Check that a role with given class name does not yet exist
  $roles = user_roles(TRUE);

  foreach($roles as $role_name){
    if (!isset($organization_role->rid) && $role_name == $organization_role->name){
      form_set_error('fa_invite_organization', t('Role name with given organization already exists'));
      return;
    }
    if (!isset($product_role->rid) && $role_name == $product_role->name){
      form_set_error('fa_invite_product', t('Role name with given product already exists'));
      return;
    }
  }

  if (!valid_url($form_state['values']['crm_link'], TRUE)){
    form_set_error('crm_link', t('Not a valid URL'));
    return;
  }
}

function fa_course_admin_edit_form_submit($form, &$form_state) {
  $course = $form_state['course'];

  // Organization role
  $organization_role = _fa_course_get_selected_role($form_state, 'organization');
  if (!isset($organization_role->rid)){
    // Organization role needs to be saved
    user_role_save($organization_role);
  }
  $course->organization_id = $organization_role->rid;

  // Product role
  $product_role = _fa_course_get_selected_role($form_state, 'product');
  if (!isset($product_role->rid)){
    // Product role needs to be saved
    user_role_save($product_role);
  }
  $course->product_id = $product_role->rid;

  $course->city = $form_state['values']['city'];
  $course->crm_link = $form_state['values']['crm_link'];
  $course->lead_sales_id = $form_state['values']['lead_sales'];
  if ($form_state['values']['lead_trainer'] != 0)
    $course->lead_trainer_id = $form_state['values']['lead_trainer'];

  if (isset($course->facid))
    drupal_write_record('fa_course', $course, 'facid');
  else
    drupal_write_record('fa_course', $course);
  drupal_set_message(t('Course @facid saved.', array('@facid' => $course->facid)));
  $form_state['redirect'] = "admin/people/courses";
}

function _get_session_types() {
  return array(
      0 => t("Other"),
      1 => t("Lecture"),
      2 => t("Workshop"),
      3 => t("Lecture and Workshop"),
      4 => t("Discussion"),
      5 => t("One on One"),
  );
}

function fa_course_admin_delete($facid){
  $links = '<p>' . l(t("Back to courses"), "admin/people/courses") . '</p>';
  $output = render($links);
  $delete_form = drupal_get_form('fa_course_admin_delete_form', $facid);
  $output .= render($delete_form);
  return $output;
}

function fa_course_admin_delete_form($form, &$form_state, $facid) {
  $form['facid'] = array(
    '#type' => 'value',
    '#value' => $facid,
  );

  return confirm_form(
    $form,
    t('Are you sure you want to delete the course %facid?', array('%facid' => $facid)),
    "admin/people/courses",
    t('This action cannot be undone.'),
    t('Delete'),
    t('Cancel')
  );
}

function fa_course_admin_delete_form_validate($form, &$form_state) {
  $facid = $form_state['values']['facid'];

  // Course with sessions can't be deleted
  $query = db_select('fa_course_session')
          ->condition('facid', $facid);
  $query->addExpression('COUNT(*)');
  $session_count = $query
                    ->execute()
                    ->fetchField();
  if ($session_count){
    form_set_error('submit', t("You must first delete the %count sessions in this course", array('%count' => $session_count)));
  }

  // Course with participants can't be deleted
  $query = db_select('fa_course_participant')
          ->condition('facid', $facid);
  $query->addExpression('COUNT(*)');
  $participant_count = $query
                    ->execute()
                    ->fetchField();
  if ($participant_count){
    form_set_error('submit', t("You must first delete the %count participants in this course", array('%count' => $participant_count)));
  }
}

function fa_course_admin_delete_form_submit($form, &$form_state) {
  $facid = $form_state['values']['facid'];

  db_delete('fa_course')
    ->condition('facid', $facid)
    ->execute();

  drupal_set_message(t('Course with id %facid has been deleted.', array('%facid' => $facid)));
  $form_state['redirect'] = "admin/people/courses";
}

/**
 * Return a list of all course sessions.
 */
function fa_course_admin_sessions($facid) {
  $header = array(
      array('data' => t('Session Id'), 'field' => 'facsid'),
      t('Session type'),
      array('data' => t('Start time'), 'field' => 'session_start_datetime', 'sort' => 'asc'),
      t('End Time'),
      t('Address'),
      t('Material'),
      t('Assistant email'),
      t('Participants'),
      t('Actions'),
  );

  $session_type = _get_session_types();

  $result = db_select('fa_course_session', 's')
    ->fields('s', array('facid', 'facsid', 'session_type', 'session_start_datetime', 'session_end_datetime', 'address', 'material', 'assistant_email'))
    ->extend('TableSort')
    ->extend('PagerDefault');
  $result->leftJoin('fa_sessions_participants', 'sp', 'sp.facsid = s.facsid');
  $result->addExpression('COUNT(sp.facpid)', 'participants');
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
    $cells[] = $row->participants;
    $cells[] = (user_access('administer courses'))
                ? l(t('Edit'), "admin/people/courses/sessions/$facid/edit/$row->facsid") . '<br/>' .
                  l(t('Delete'), "admin/people/courses/sessions/$facid/delete/$row->facsid")
                : "";
    $rows[] = $cells;
  }

  $links = '<p>' . l(t("Back to courses"), "admin/people/courses") . '<br/>'
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

function fa_course_admin_sessions_edit($facid, $facsid){
  $links = '<p>' . l(t("Back to sessions"), "admin/people/courses/sessions/$facid") . '</p>';
  $output = render($links);


  $course_session = _get_course_session($facid, $facsid);
  $edit_form = drupal_get_form('fa_course_admin_session_edit_form', $course_session);
  $output .= render($edit_form);
  return $output;
}

function _get_course_session($facid, $facsid){
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


function _get_assignment_options($product_name, $node_type){
  $result = db_query("SELECT n.nid, n.title from node n INNER JOIN url_alias ua ON ua.source = CONCAT('node/', n.nid) "
      . "WHERE n.type = :type AND ua.alias LIKE :match",
      array(':type' => $node_type,
            ':match' => 'lykeion/' . strtolower($product_name) . '%'));
  $assignment_options = array();
  foreach($result as $row){
    $assignment_options[$row->nid] = $row->title;
  }
  return $assignment_options;
}

/**
 * Form for creating/editing a course session.
 */
function fa_course_admin_session_edit_form($form, &$form_state, $course_session) {
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
  );
  $form['course_session']['facid'] = array(
      '#type' => 'textfield',
      '#title' => t('Course Id'),
      '#disabled' => TRUE,
      '#value' => $course_session->facid,
  );

  $session_type = _get_session_types();

  $form['course_session']['session_type'] = array(
      '#type' => 'select',
      '#title' => t('Session type'),
      '#default_value' => isset($course_session->session_type) ? $course_session->session_type : 1,
      '#options' => _get_session_types(),
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
  );

  $form['course_session']['address'] = array(
      '#type'           => 'textfield',
      '#title'          => t('Address of session'),
      '#description'          => t('Detailed address for the session, should include all information that is
                                    required for both the participants and the trainer to find their way to the
                                    right place. E.g. room number and contact phone number might be needed.'),
      '#default_value'  => isset($course_session->address) ? $course_session->address : NULL,
  );

  $form['course_session']['material'] = array(
      '#type'           => 'textfield',
      '#title'          => t('Material promised to the session'),
      '#default_value'  => isset($course_session->material) ? $course_session->material : NULL,
  );

  $form['course_session']['summary_emails'] = array(
      '#type'           => 'textfield',
      '#title'          => t('Summary emails, separated with comma'),
      '#default_value'  => isset($course_session->summary_emails) ? $course_session->summary_emails : NULL,
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

  $preliminary_assignment_options = _get_assignment_options($product_name, 'preliminary_assignment');
  $form['course_session'] = $form['course_session'] + _get_assignment_form($course_session,
                                                'preliminary_assignment',
                                                'preliminary assignment',
                                                $preliminary_assignment_options,
                                                $product_name);
  // Course assignment

  $course_assignment_options = _get_assignment_options($product_name, 'course_assignment');
  $form['course_session'] = $form['course_session'] + _get_assignment_form($course_session,
                                                'course_assignment',
                                                'course assignment',
                                                $course_assignment_options,
                                                $product_name);
  // Feedback

  $feedback_options = _get_assignment_options($product_name, 'course_feedback');
  $form['course_session'] = $form['course_session'] + _get_assignment_form($course_session,
                                                'feedback',
                                                'feedback',
                                                $feedback_options,
                                                $product_name);

  $form['course_session']['submit'] = array(
      '#type' => 'submit',
      '#value' => t('Save'),
  );

  $form_state['course_session'] = $course_session;
  return $form;
}

function _get_assignment_form($course_session, $assignment_type, $assignment_type_name, $assignment_options, $product_name) {
  $form = array();

  $assignment_sent_datetime = $assignment_type . '_sent';
  $assignment_sent = isset($course_session->$assignment_sent_datetime);

  $form[$assignment_type] = array(
      '#type' => 'fieldset',
      '#title' => ucfirst($assignment_type_name),
  );

  $assignment_type_node_id = $assignment_type . '_node_id';
  $form[$assignment_type][$assignment_type . '_check'] = array(
      '#type'           => 'checkbox',
      '#title'          => t('This session has a ' . $assignment_type_name),
      '#disabled'       => $assignment_sent || empty($assignment_options),
      '#default_value'  => isset($course_session->$assignment_type_node_id) ? TRUE : FALSE,
  );

  $form[$assignment_type][$assignment_type . '_select'] = array(
      '#type' => 'select',
      '#title' => t(ucfirst($assignment_type_name) . ' form'),
      '#default_value' => isset($course_session->$assignment_type_node_id) ?
                                  $course_session->$assignment_type_node_id :
                                    reset($assignment_options),
      '#options' => $assignment_options,
      '#disabled' => $assignment_sent,
      '#states' => array(
          'visible' => array(
              ':input[name=' . $assignment_type . '_check]' => array('checked' => TRUE),
          ),
      ),
  );

  $template = array();
  $assignment_email_template = $assignment_type . '_email_template';
  if (isset($course_session->$assignment_email_template))
    $template = unserialize($course_session->$assignment_email_template);

  $form[$assignment_type][$assignment_type . '_subject'] = array(
      '#type'           => 'textfield',
      '#title'          => t(ucfirst($assignment_type_name) . ' email title'),
      '#default_value'  => !empty($template) ? $template['subject'] : $product_name . ' # : ' . ucfirst($assignment_type_name),
      '#disabled'       => $assignment_sent,
      '#states' => array(
          'visible' => array(
              ':input[name=' . $assignment_type . '_check]' => array('checked' => TRUE),
          ),
      ),
  );

  $form[$assignment_type][$assignment_type . '_message'] = array(
      '#type'           => 'textarea',
      '#title'          => t(ucfirst($assignment_type_name) . ' email body'),
      '#default_value'  => !empty($template) ? $template['message'] : '',
      '#disabled'       => $assignment_sent,
      '#states' => array(
          'visible' => array(
              ':input[name=' . $assignment_type . '_check]' => array('checked' => TRUE),
          ),
      ),
  );

  $assignment_datetime = $assignment_type . '_datetime';
  $form[$assignment_type][$assignment_type . '_action'] = array(
      '#type'           => 'radios',
      '#title'          => t(ucfirst($assignment_type_name) . ' action'),
      '#access'         => !$assignment_sent,
      '#options'        => array(SCHEDULE => t('Schedule ' . $assignment_type_name),
                                 SEND_NOW => t('Send ' . $assignment_type_name . ' now')),
      '#default_value'  => isset($course_session->$assignment_datetime) ?
                            SCHEDULE : NULL,
      '#states' => array(
          'visible' => array(
              ':input[name=' . $assignment_type . '_check]' => array('checked' => TRUE),
          ),
      ),
  );

  $default_assignment_datetime = date('Y-m-d H:i', REQUEST_TIME);
  if (isset($course_session->session_start_datetime)){
    $default_assignment_datetime = date('Y-m-d H:i', $course_session->session_start_datetime - (SECONDS_IN_DAY * 14));
  }

  $form[$assignment_type][$assignment_type . '_schedule_datetime'] = array(
      '#type'           => 'date_popup',
      '#title'          => t('Date and time when the ' .  $assignment_type_name . ' should be sent'),
      '#access'         => !$assignment_sent,
      '#date_format'    => DATE_FORMAT,
      '#default_value'  => isset($course_session->$assignment_datetime) ? date('Y-m-d H:i', $course_session->$assignment_datetime) :
                                 $default_assignment_datetime,
      '#states' => array(
          'visible' => array(
              ':input[name=' . $assignment_type . '_check]' => array('checked' => TRUE),
              ':input[name=' . $assignment_type . '_action]' => array('value' => SCHEDULE),
              ),
      ),
  );

  $form[$assignment_type]['sent'] = array(
      '#type' => 'textfield',
      '#title' => t(ucfirst($assignment_type_name) . ' sent time'),
      '#disabled' => TRUE,
      '#access' => $assignment_sent,
      '#value' => isset($course_session->$assignment_sent_datetime) ?
                      format_date($course_session->$assignment_sent_datetime, 'custom', DATE_FORMAT) : NULL,
      '#states' => array(
          'visible' => array(
              ':input[name=' . $assignment_type . '_check]' => array('checked' => TRUE),
          ),
      ),
  );

  $reminder_sent_datetime = $assignment_type . '_reminder_sent';
  $assignment_reminder_datetime = $assignment_type . '_reminder_datetime';
  $form[$assignment_type][$assignment_type . '_reminder_action'] = array(
      '#type'           => 'radios',
      '#title'          => t(ucfirst($assignment_type_name) . ' reminder action'),
      '#access'         => !isset($course_session->$reminder_sent_datetime),
      '#options'        => array(SCHEDULE => t('Schedule ' . $assignment_type_name . ' reminder'),
                                 SEND_NOW => t('Send ' . $assignment_type_name . ' reminder now')),
      '#default_value'  => isset($course_session->$assignment_reminder_datetime) ?
                            SCHEDULE : NULL,
      '#states' => array(
          'visible' => array(
              ':input[name=' . $assignment_type . '_check]' => array('checked' => TRUE),
          ),
      ),
  );

  $default_reminder_datetime = date('Y-m-d H:i', REQUEST_TIME);
  if (isset($course_session->session_start_datetime)){
    $default_reminder_datetime = date('Y-m-d H:i', $course_session->session_start_datetime - (SECONDS_IN_DAY * 2));
  }

  $form[$assignment_type][$assignment_type . '_reminder_schedule_datetime'] = array(
      '#type'           => 'date_popup',
      '#title'          => t('Date and time when a reminder for the ' .  $assignment_type_name . ' should be sent'),
      '#access'         => !isset($course_session->$reminder_sent_datetime),
      '#date_format'    => DATE_FORMAT,
      '#default_value'  => isset($course_session->$assignment_datetime) ? date('Y-m-d H:i', $course_session->$assignment_reminder_datetime) :
                                 $default_reminder_datetime,
      '#states' => array(
          'visible' => array(
              ':input[name=' . $assignment_type . '_check]' => array('checked' => TRUE),
              ':input[name=' . $assignment_type . '_reminder_action]' => array('value' => SCHEDULE),
              ),
      ),
  );

  $form[$assignment_type]['reminder_sent'] = array(
      '#type' => 'textfield',
      '#title' => t(ucfirst($assignment_type_name) . ' reminder sent time'),
      '#disabled' => TRUE,
      '#access' => isset($course_session->$reminder_sent_datetime),
      '#value' => isset($course_session->$assignment_reminder_datetime) ?
                      format_date($course_session->$assignment_reminder_datetime, 'custom', DATE_FORMAT) : NULL,
      '#states' => array(
          'visible' => array(
              ':input[name=' . $assignment_type . '_check]' => array('checked' => TRUE),
          ),
      ),
  );

  return $form;
}



function fa_course_admin_session_edit_form_validate($form, &$form_state) {

  if (!empty($form_state['values']['assistant_email']) && valid_email_address($form_state['values']['assistant_email'])){
    form_set_error('assistant_email', t('Not a valid email address'));
    return;
  }
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

  $summary_emails = _invite_get_emails($form_state['values']['summary_emails']);
  _fa_course_validate_emails('summary_emails');

  // Check assistant email
  $assistant_email = _invite_get_emails($form_state['values']['assistant_email']);
  if (_fa_course_validate_emails('assistant_email') == FALSE){
    return;
  }

  // Preliminary assignment
  _validate_assignment_schedule($form_state, 'preliminary_assignment');

  // Course assignment
  _validate_assignment_schedule($form_state, 'course_assignment');

  // Feedback
  _validate_assignment_schedule($form_state, 'feedback');

}

function _validate_assignment_schedule($form_state, $assignment_type){
  if ($form_state['values'][$assignment_type . '_action'] == SCHEDULE){
    $assignment_datetime = strtotime($form_state['values'][$assignment_type . '_schedule_datetime']);
    if ($assignment_datetime < strtotime("now")){
      form_set_error($assignment_type . '_schedule_datetime', t('Schedule must be in the future.'));
    }
  }

  if ($form_state['values'][$assignment_type . '_reminder_action'] == SEND_NOW){
    $assignment_sent = $assignment_type . '_sent';
    if (!isset($form_state['course_session']->$assignment_sent)){
      form_set_error($assignment_type . '_reminder_action', t('Can not send reminder before assignment!'));
    }
  }elseif($form_state['values'][$assignment_type . '_reminder_action'] == SCHEDULE){
    $reminder_datetime = strtotime($form_state['values'][$assignment_type . '_reminder_schedule_datetime']);
    if ($reminder_datetime < strtotime("now")) {
      form_set_error($assignment_type . 'reminder_schedule_datetime', t('Schedule must be in the future.'));
    }
    if (isset($assignment_datetime) && $reminder_datetime < $assignment_datetime){
      form_set_error($assignment_type . '_reminder_schedule_datetime', t('Reminder schedule must be after the assignment schedule.'));
    }
  }
}

function fa_course_admin_session_edit_form_submit($form, &$form_state) {
  $course_session = $form_state['course_session'];

  $course_session->session_type = $form_state['values']['session_type'];
  $course_session->session_start_datetime = $form_state['values']['start_datetime_epoch'];
  $course_session->session_end_datetime = $form_state['values']['end_datetime_epoch'];
  $course_session->address = $form_state['values']['address'];
  $course_session->material = $form_state['values']['material'];
  $course_session->summary_emails = $form_state['values']['summary_emails'];
  $course_session->assistant_email = $form_state['values']['assistant_email'];

  _store_assignment($course_session, $form_state, 'preliminary_assignment');
  _store_assignment($course_session, $form_state, 'course_assignment');
  _store_assignment($course_session, $form_state, 'feedback');

  if (isset($course_session->facsid))
    drupal_write_record('fa_course_session', $course_session, 'facsid');
  else
    drupal_write_record('fa_course_session', $course_session);
  drupal_set_message(t('Course session @facsid saved.', array('@facsid' => $course_session->facsid)));
  $form_state['redirect'] = "admin/people/courses/sessions/" . $course_session->facid;
}

function _store_assignment($course_session, $form_state, $assignment_type){
  // Save assignments only if they haven't been sent already
  $assignment_sent = $assignment_type . '_sent';
  if ($form_state['values'][$assignment_type . '_check'] && !isset($course_session->$assignment_sent)){
    $assignment_node_id = $assignment_type . '_node_id';
    $course_session->$assignment_node_id = $form_state['values'][$assignment_type . '_select'];
    $template = array();
    $template['subject'] = $form_state['values'][$assignment_type . '_subject'];
    $template['message'] = $form_state['values'][$assignment_type . '_message'];
    $assignment_email_template = $assignment_type . '_email_template';
    $course_session->$assignment_email_template = serialize($template);
    if (isset($form_state['values'][$assignment_type . '_action']))
      if ($form_state['values'][$assignment_type . '_action'] == SCHEDULE){
        $assignment_datetime = $assignment_type . '_datetime';
        $course_session->$assignment_datetime = strtotime($form_state['values'][$assignment_type . '_schedule_datetime']);
      }else{
        dpm("Immediate send not implemented!");
      }
  }
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

function fa_course_admin_sessions_delete($facid, $facsid){
  $links = '<p>' . l(t("Back to sessions"), "admin/people/courses/sessions/$facid") . '</p>';
  $output = render($links);
  $session = _get_course_session($facid, $facsid);
  $delete_form = drupal_get_form('fa_course_admin_sessions_delete_form', $session);
  $output .= render($delete_form);
  return $output;
}

function fa_course_admin_sessions_delete_form($form, &$form_state, $session) {
  $form['session'] = array(
    '#type' => 'value',
    '#value' => $session,
  );

  return confirm_form(
    $form,
    t('Are you sure you want to delete the session %facsid?', array('%facsid' => $session->facsid)),
    "admin/people/courses/sessions/" . $session->facid,
    t('This action cannot be undone.'),
    t('Delete'),
    t('Cancel')
  );
}

function fa_course_admin_sessions_delete_form_submit($form, &$form_state) {
  $session = $form_state['values']['session'];

  db_delete('fa_course_session')
    ->condition('facsid', $session->facsid)
    ->execute();

  drupal_set_message(t('Session with id %facsid has been deleted.', array('%facsid' => $session->facsid)));
  $form_state['redirect'] = "admin/people/courses/sessions/" . $session->facid;
}

/**
 * Return a list of all course participants.
 */
function fa_course_admin_participants($facid) {
  $header = array(
    array('data' => t('E-mail'), 'field' => 'p.email', 'sort' => 'asc'),
    array('data' => t('First Name'), 'field' => 'p.first_name'),
    array('data' => t('Last Name'), 'field' => 'p.last_name'),
    t('Invite sent'),
    t('Joined'),
    t('Invite reuses left'),
    t('Actions'),
  );

  $result = db_select('fa_course_participant', 'p')
    ->fields('p', array('facid', 'facpid', 'email', 'first_name', 'last_name', 'invite_reuse', 'iid', 'uid'))
    ->extend('TableSort')
    ->extend('PagerDefault');
  $result->condition('p.facid', $facid);
  $result->leftJoin('invite', 'i', 'i.iid = p.iid');
  $result->addField('i', 'created', 'invite_sent');
  $result->addField('i', 'joined', 'invite_joined');
  $result->distinct();
  $result->groupBy('p.facid');
  $result->groupBy('p.facpid');

  $result->orderByHeader($header);
  $result = $result->execute()->fetchAll();

  $rows = array();
  foreach ($result as $row) {
    $cells = array();
    $cells[] = isset($row->uid) ? l($row->email, "user/" . $row->uid) : $row->email;
    $cells[] = isset($row->first_name) ? $row->first_name : t('Not set');
    $cells[] = isset($row->last_name) ? $row->last_name : t('Not set');
    $cells[] = $row->invite_sent;
    $cells[] = $row->invite_joined;
    $cells[] = $row->invite_reuse;
	$cells[] = (user_access('administer courses'))
						? l(t('Edit'), "admin/people/courses/participants/$facid/edit/$row->facpid") . '<br/>' .
						  l(t('Delete'), "admin/people/courses/participants/$facid/delete/$row->facpid")
						: "";
    $rows[] = $cells;
  }

  $links = '<p>' . l(t("Back to courses"), "admin/people/courses") . '<br/>'
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

function fa_course_admin_participants_edit($facid, $facpid){
  $links = '<p>' . l(t("Back to participants"), "admin/people/courses/participants/$facid") . '</p>';
  $output = render($links);

  $participant = _get_participant($facid, $facpid);
  $edit_form = drupal_get_form('fa_course_admin_participants_edit_form', $participant);
  $output .= render($edit_form);
  return $output;
}

function _get_participant($facid, $facpid){
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
function fa_course_admin_participants_edit_form($form, &$form_state, $participant) {
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

  $form['participant']['submit'] = array(
      '#type' => 'submit',
      '#value' => t('Save'),
  );

  $form_state['participant'] = $participant;
  return $form;
}

function fa_course_admin_participants_edit_form_validate($form, &$form_state) {
  // Check email
  $email = _invite_get_emails($form_state['values']['email']);
  if (_fa_course_validate_emails('email') == FALSE){
    return;
  }

  // Check that email is unique for this course
  if (db_select('fa_course_participant', 'p')
          ->fields('p')
          ->condition('p.email', $form_state['values']['email'])
          ->condition('p.facid', $form_state['participant']->facid)
          ->execute()->fetchObject() != FALSE){
    form_set_error('email', t('Course already has a participant with given email address'));
  }
}

function fa_course_admin_participants_edit_form_submit($form, &$form_state) {
  $participant = $form_state['participant'];
  $participant->first_name = $form_state['values']['first_name'];
  $participant->last_name = $form_state['values']['last_name'];
  $participant->email = $form_state['values']['email'];

  if (isset($participant->facpid))
    drupal_write_record('fa_course_participant', $participant, 'facpid');
  else
    drupal_write_record('fa_course_participant', $participant);
  drupal_set_message(t('Participant @facpid saved.', array('@facpid' => $participant->facpid)));
  $form_state['redirect'] = "admin/people/courses/participants/" . $participant->facid;
}

function fa_course_admin_participants_delete($facid, $facpid){
  $links = '<p>' . l(t("Back to participants"), "admin/people/courses/participants/$facid") . '</p>';
  $output = render($links);
  $participant = _get_participant($facid, $facpid);
  $delete_form = drupal_get_form('fa_course_admin_participants_delete_form', $participant);
  $output .= render($delete_form);
  return $output;

}

function fa_course_admin_participants_delete_form($form, &$form_state, $participant) {
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

function fa_course_admin_participants_delete_form_submit($form, &$form_state) {
  $participant = $form_state['values']['participant'];

	db_delete('fa_course_participant')
	  ->condition('facpid', $participant->facpid)
	  ->execute();

  drupal_set_message(t('Participant with email %email has been deleted.', array('%email' => $participant->email)));
  $form_state['redirect'] = "admin/people/courses/participants/" . $participant->facid;
}


function fa_course_admin_participants_import($facid){
  $links = '<p>' . l(t("Back to participants"), "admin/people/courses/participants/$facid") . '</p>';
  $output = render($links);

  $import_form = drupal_get_form('fa_course_admin_participants_import_form', $facid);
  $output .= render($import_form);
  return $output;
}


/**
 * Form for importing course participants.
 */
function fa_course_admin_participants_import_form($form, &$form_state, $facid) {
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
