<?php

/**
 * @file
 * Administration functions for Filosofian Akatemia Course module.
 */

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
  $result->addField('us', 'name', 'sales');
  $result->leftJoin('users', 'ut', 'ut.uid = fac.lead_trainer_id');
  $result->addField('ut', 'name', 'trainer');
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
    $cells[] = isset($row->first_session) ? format_date($row->first_session, 'short') : t('Not set');
    $cells[] = str_replace('#', '', $row->organization);
    $cells[] = str_replace('#', '', $row->product);
    $cells[] = l($row->sales, "user/" . $row->lead_sales_id);
    $cells[] = isset($row->trainer) ? l($row->trainer, "user/" . $row->lead_trainer_id) : t('Not set');
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
    $cells[] = $phase['title'] . (isset($phase['action']) ? " <br/> " . $phase['action'] : "");
    $cells[] = $row->sessions . "<br/>" . l(t("Edit sessions"), "admin/people/courses/sessions/$row->facid");
    $cells[] = $row->participants . "<br/>" . l(t("Edit participants"), "admin/people/courses/participants/$row->facid");
    $cells[] = l(t('Edit'), "admin/people/courses/edit/$row->facid");
    $rows[] = $cells;
  }

  $add_new = '<p>' . l(t("Add new course"), "admin/people/courses/edit/new") . '</p>';
  $output = render($add_new);

  $output .= theme('table', array('header' => $header, 'rows' => $rows));
  if (!$rows) {
    $output .= t('No courses found.');
  }
  else {
    $output .= theme('pager');
  }

  return $output;
}

function fa_course_admin_edit($facid){
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

  $edit_form = drupal_get_form('fa_course_admin_edit_form', $course);
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
function fa_course_admin_edit_form($form, &$form_state, $course) {
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
      '#description'    => t('Select the organization/company where the course will be kept.'),
  );
  $form['course']['organization'] = array(
      '#type'           => 'textfield',
      '#title'          => t('New Organization'),
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
      '#options'        => $product_options,
      '#description'    => t('Select the product.'),
  );
  $form['course']['product'] = array(
      '#type'           => 'textfield',
      '#title'          => t('Custom product'),
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
      '#default_value' => isset($course->city) ? $course->city: 'Helsinki',
      '#required' => TRUE,
  );
  $form['course']['crm_link'] = array(
      '#type' => 'textfield',
      '#title' => t('CRM Link'),
      '#default_value' => isset($course->crm_link) ? $course->crm_link: NULL,
      '#required' => TRUE,
  );
  $form['course']['lead_sales'] = array(
      '#type'           => 'select',
      '#multiple'       => FALSE,
      '#title'          => t('Lead Sales'),
      '#options'        => $academians,
      '#default_value'  => isset($course->lead_sales_id) ? $course->lead_sales_id :
                          (isset($academians[$user->uid]) ? $user->uid : NULL),
  );
  $form['course']['lead_trainer'] = array(
      '#type'           => 'select',
      '#multiple'       => FALSE,
      '#title'          => t('Lead Trainer'),
      '#options'        => $academians,
      '#default_value'  => isset($course->lead_trainer_id) ? $course->lead_trainer_id : 0,
  );
  $form['course']['submit'] = array(
      '#type' => 'submit',
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

/**
 * Return a list of all course sessions.
 */
function fa_course_admin_sessions($facid) {
  $header = array(
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
    $cells[] = $session_type[$row->session_type];
    $cells[] = format_date($row->session_start_datetime, 'short');
    $cells[] = format_date($row->session_end_datetime, 'short');
    $cells[] = $row->address;
    $cells[] = $row->material;
    $cells[] = $row->assistant_email;
    $cells[] = $row->participants;
    $cells[] = l(t('Edit'), "admin/people/courses/sessions/$facid/edit/$row->facsid");
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
  $output .= t('TODO: edit course ' . $facid . ' session ' . $facsid);

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

  $edit_form = drupal_get_form('fa_course_admin_session_edit_form', $course_session);
  $output .= render($edit_form);
  return $output;
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
      '#value' => isset($course_session->facsid) ? $course->facsid : 'N/A',
  );
  $form['course_session']['facid'] = array(
      '#type' => 'textfield',
      '#title' => t('Course Id'),
      '#disabled' => TRUE,
      '#value' => $course_session->facid,
  );

  $session_type = _get_session_types();

  $form['course_session']['session_type'] = array(
      '#type' => 'radios',
      '#title' => t('Choose Course Action'),
      '#default_value' => isset($course_session->session_type) ? $course_session->session_type : 1,
      '#options' => _get_session_types(),
  );

  // Time of course
  $form['course_session']['start_datetime'] = array(
      '#type'           => 'date',
      '#title'          => t('Start date and time for the session'),
      '#default_value'  => isset($course_session->start_timestamp) ? $course_session->start_timestamp : REQUEST_TIME,
      '#required'       => TRUE,
  );

  // Length of coursed
  $form['course_session']['length'] = array(
      '#type'           => 'textfield',
      '#title'          => t('Length of the session in minutes'),
      '#default_value'  => isset($course_session->end_timestamp) ? ($course_session->end_timestamp - $course_session->start_timestamp) / 1000 / 60: 180,
      '#required'       => TRUE,
  );

  $form['course_session']['submit'] = array(
      '#type' => 'submit',
      '#value' => t('Save'),
  );

  $form_state['course_session'] = $course_session;
  return $form;
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
    $cells[] = l(t('Edit'), "admin/people/courses/participants/$facid/edit/$row->facpid");
    $rows[] = $cells;
  }

  $links = '<p>' . l(t("Back to courses"), "admin/people/courses") . '<br/>'
             . l(t("Add new participants"), "admin/people/courses/participants/$facid/edit/new") . '</p>';
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

  $output .= t('TODO: edit course ' . $facid . ' participant ' . $facpid);
  return $output;
}