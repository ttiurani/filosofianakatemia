<?php

/**
 * @file
 * Course editing functions for Filosofian Akatemia Course module.
 */

/**
 * Return a list of all courses.
 */
function fa_course_overview() {
  $header = array(
    array('data' => t('Course Id'), 'field' => 'facid', 'sort' => 'desc'),
    t('Course name'),
    array('data' => t('First session'), 'field' => 'first_session'),
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
  );

  $result = db_select('fa_course', 'fac')
    ->fields('fac', array('facid', 'city', 'crm_link', 'trainer_handover', 'sales_handover', 'lead_sales_id', 'lead_trainer_id'))
    ->extend('TableSort')
    ->extend('PagerDefault')
    ->limit(20);
  $result->innerJoin('role', 'ro', 'ro.rid = fac.organization_id');
  $result->addField('ro', 'name', 'organization');
  $result->addField('ro', 'rid', 'organization_id');
  $result->innerJoin('role', 'rp', 'rp.rid = fac.product_id');
  $result->addField('rp', 'name', 'product');
  $result->addField('rp', 'rid', 'product_id');
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
    $course_name = _fa_course_format_course_name($row->product_id, $row->organization_id, (isset($row->first_session) ? $row->first_session : NULL));
    $cells[] = l($course_name, "admin/people/courses/view/$row->facid");
    $cells[] = isset($row->first_session) ? format_date($row->first_session, 'custom', 'D d.m.Y H:i') : t('Not set');
    $cells[] = str_replace('#', '', $row->organization);
    $cells[] = str_replace('#', '', $row->product);
    $cells[] = l($row->sales_first_name . " " . $row->sales_last_name, "user/" . $row->lead_sales_id);
    $cells[] = isset($row->trainer_first_name) ? l($row->trainer_first_name . " " . $row->trainer_last_name, "user/" . $row->lead_trainer_id) : t('Not set');
    $cells[] = $row->city;
    $cells[] = $row->course_role == NULL ? t('Not set') : $row->course_role;
    $cells[] = l(t("View CRM"), $row->crm_link, array('attributes' => array('target'=>'_blank')));

    $phase = _fa_course_get_phase($row);
    $cells[] = $phase['title'];
    $cells[] = $row->sessions;
    $cells[] = $row->participants;
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

function _fa_course_get_phase($course){
  $phase_options =  array(
        'pending' =>  array(
            'title' => t("Sales pending more information"),
            'action' => l(t("Handover to the trainer"), "admin/people/courses/trainer_handover/$course->facid")),
        'trainer' =>  array(
            'title' => t("Trainer responsibility"),
            'action' => l(t("Handover back to sales"), "admin/people/courses/sales_handover/$course->facid")),
        'finished' =>  array(
            'title' => t("Sales responsible for resale")),
  );

  $phase = $phase_options['pending'];
  if (isset($course->trainer_handover)){
    $phase = $phase_options['trainer'];
    if (isset($course->sales_handover)){
      $phase = $phase_options['finished'];
    }
  }
  return $phase;
}


function fa_course_details($action, $facid){
  module_load_include('inc', 'fa_course', 'fa_course.capsulecrm');

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
  $links = '<p><strong>' . _fa_course_get_course_name($facid) . '</strong><br/>';
  if ($action == 'view'){
    $links .= l(t("Back"), "admin/people/courses")
               . (user_access('administer courses')
                 ? " <br/> " . l(t('Edit'), "admin/people/courses/edit/$facid") . '<br/>'
                   . l(t('Delete'), "admin/people/courses/delete/$facid")
                 : "");
  }elseif ($action == 'edit'){
    $links .= l(t("Back"), "admin/people/courses/view/$facid");
  }
  $links .= '</p>';

  $opportunity = NULL;
  if (isset($course->facid) && !isset($course->sales_handover)){
    $opportunity = fa_course_get_crm_opportunity($course->crm_link);
  }


  $output = render($links);
  $info_form = drupal_get_form('fa_course_info_form', $course, $action, $opportunity);
  $output .= render($info_form);
  $edit_form = drupal_get_form('fa_course_edit_form', $course, $action, $opportunity);
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
 * Form for course static information.
 */
function fa_course_info_form($form, &$form_state, $course, $action, $opportunity) {

  $info = _fa_course_get_course_info(isset($course->facid) ? $course->facid : NULL);
  $form = array();
  if ($info != FALSE){
    $form['course_info'] = array(
        '#type' => 'fieldset',
        '#title' => t('Course Info'),
    );
    $form['course_info']['participants'] = array(
        '#markup' => '<p>' . l('Participants: ' . $info->participants, "admin/people/courses/participants/$course->facid") . '<br/>'
                           . l('Sessions: ' . $info->sessions, "admin/people/courses/sessions/$course->facid")
                     . '</p>',
    );

    $phase = _fa_course_get_phase($info);

    $form['course_info']['phase'] = array(
        '#markup' => '<p>' . t('Phase: ') . $phase['title'],
    );

    if ($opportunity != NULL){
      if ($opportunity->probability == '100'){
        if (isset($course->lead_trainer_id)){
          $form['course_info']['phase']['#markup'] .= (isset($phase['action']) && user_access('administer courses') ?
                                                    '<br/>' . $phase['action'] : "")
                                                   . '</p>';
        }
      }elseif (isset($phase['action'])){
        $form['course_info']['phase']['#markup'] .= '<br/><strong>' . t("Can't handover, opportunity probability not 100%")
                                                   . '</strong></p>';
      }
      $form['course_info']['crm'] = array(
          '#markup' => '<p>' . t('Opportunity name: ') . $opportunity->name . '<br/>'
                             . t('Probablity: ') . $opportunity->probability . '%' . '<br/>'
                             . t('Value: ') . $opportunity->value . ' ' . $opportunity->currency . '<br/>'
                      . '</p>',
      );
    }else{
      $form['course_info']['phase']['#markup'] .= '</p>';
    }
  }
  return $form;
}

function _fa_course_get_course_info($facid){
  if ($facid != NULL){
    $result = db_query(
                    "SELECT c.facid AS facid, c.sales_handover AS sales_handover, c.trainer_handover AS trainer_handover, "
                  .         "COUNT(DISTINCT s.facsid) AS sessions, COUNT(DISTINCT p.facpid) as participants, c.crm_link AS crm_link "
                  . "FROM {fa_course} c "
                  . "LEFT JOIN {fa_course_session} s ON s.facid = c.facid "
                  . "LEFT JOIN {fa_course_participant} p ON p.facid = c.facid "
                  . "WHERE c.facid = :facid",
                    array(':facid' => $facid));
    foreach($result as $row){
      return $row;
    }
  }
  return FALSE;
}

/**
 * Form for creating/editing a course.
 */
function fa_course_edit_form($form, &$form_state, $course, $action, $opportunity) {
  global $user;

  $academians = _fa_course_get_academians();
  if (!isset($course->lead_trainer_id))
    $academians[0] = t('-- unknown --');

  $course_roles = _fa_course_get_course_roles();

  // Not listed options
  $course_roles['organizations'][0] = t('--not listed--');
  $course_roles['products'][0] = t('--not listed--');

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
  $form['course']['trainer_handover'] = array(
      '#type' => 'textfield',
      '#title' => t('Handed over to the trainer'),
      '#disabled' => TRUE,
      '#access' => isset($course->trainer_handover),
      '#value' => isset($course->trainer_handover) ?
                      format_date($course->trainer_handover, 'custom', DATE_FORMAT) : NULL,
  );
  $form['course']['sales_handover'] = array(
      '#type' => 'textfield',
      '#title' => t('Handed back over to sales'),
      '#disabled' => TRUE,
      '#access' => isset($course->sales_handover),
      '#value' => isset($course->sales_handover) ?
                      format_date($course->sales_handover, 'custom', DATE_FORMAT) : NULL,
  );

  // Organization select
  $form['course']['organization_select'] = array(
      '#type'           => 'select',
      '#multiple'       => FALSE,
      '#title'          => t('Select Organization'),
      '#options'        => $course_roles['organizations'],
      '#default_value'  => isset($course->organization_id) ? $course->organization_id : NULL,
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
      '#options'        => $course_roles['products'],
      '#default_value'  => isset($course->product_id) ? $course->product_id : NULL,
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
  $form['course']['billing_information'] = array(
      '#type' => 'textarea',
      '#title' => t('Billing information'),
      '#description' => t('Information about where the invoice should be sent after the first session has been held'),
      '#disabled'       => $action == 'view',
      '#default_value' => isset($course->billing_information) ? $course->billing_information: NULL,
      '#required' => FALSE,
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

  $form['course']['trainer_evaluation'] = array(
      '#type' => 'fieldset',
      '#access' => isset($course->trainer_handover),
      '#title' => t('Trainer evaluation'),
  );

  $trainer_evaluation = array();
  if (isset($course->trainer_evaluation)){
    $trainer_evaluation = unserialize($course->trainer_evaluation);
  }
  $form['course']['trainer_evaluation']['evaluation_well'] = array(
      '#type' => 'textarea',
      '#disabled'  => $action == 'view',
      '#default_value' => isset($trainer_evaluation['well']) ? $trainer_evaluation['well']: NULL,
      '#title' => t('What went well in the training?'),
      '#access' => isset($course->trainer_handover),
  );
  $form['course']['trainer_evaluation']['evaluation_better'] = array(
      '#type' => 'textarea',
      '#disabled' => $action == 'view',
      '#default_value' => isset($trainer_evaluation['better']) ? $trainer_evaluation['better']: NULL,
      '#title' => t('What will I do better next time?'),
      '#access' => isset($course->trainer_handover),
  );
  $form['course']['trainer_evaluation']['evaluation_other'] = array(
      '#type' => 'textarea',
      '#disabled'  => $action == 'view',
      '#title' => t('Other remarks, e.g. further actions'),
      '#default_value' => isset($trainer_evaluation['other']) ? $trainer_evaluation['other']: NULL,
      '#access' => isset($course->trainer_handover),
  );
  $form['course']['other_information'] = array(
      '#type' => 'textarea',
      '#title' => t('Other information'),
      '#description' => t('Other information about the course'),
      '#disabled'       => $action == 'view',
      '#default_value' => isset($course->other_information) ? $course->other_information: NULL,
      '#required' => FALSE,
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
  if (!empty($form_state['values'][$role_type])){
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

function fa_course_edit_form_validate($form, &$form_state) {
  $organization_role = _fa_course_get_selected_role($form_state, 'organization');
  $product_role = _fa_course_get_selected_role($form_state, 'product');
  // Check that a role with given class name does not yet exist
  $roles = user_roles(TRUE);

  foreach($roles as $role_name){
    if (!isset($organization_role->rid) && $role_name == $organization_role->name){
      form_set_error('organization', t('Role name with given organization already exists'));
      return;
    }
    if (!isset($product_role->rid) && $role_name == $product_role->name){
      form_set_error('product', t('Role name with given product already exists'));
      return;
    }
  }

  if (!valid_url($form_state['values']['crm_link'], TRUE)){
    form_set_error('crm_link', t('Not a valid URL'));
    return;
  }
  if (!isset($form_state['course']->sales_handover) && !fa_course_validate_crm_opportunity_url($form_state['values']['crm_link'])){
    form_set_error('crm_link', t('Not a valid CapsuleCRM opportunity URL'));
    return;
  }
}

function fa_course_edit_form_submit($form, &$form_state) {
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
  $course->billing_information = $form_state['values']['billing_information'];
  $course->lead_sales_id = $form_state['values']['lead_sales'];
  if ($form_state['values']['lead_trainer'] != 0)
    $course->lead_trainer_id = $form_state['values']['lead_trainer'];
  $course->other_information = $form_state['values']['other_information'];

  // Trainer evaluation
  $trainer_evaluation = NULL;
  if ($form_state['values']['evaluation_well'] != NULL ||
      $form_state['values']['evaluation_better'] != NULL ||
      $form_state['values']['evaluation_other'] != NULL){
    $trainer_evaluation = serialize(array(
        'well' => $form_state['values']['evaluation_well'],
        'better' => $form_state['values']['evaluation_better'],
        'other' => $form_state['values']['evaluation_other'],
      ));
  }
  $course->trainer_evaluation = $trainer_evaluation;

  if (isset($course->facid))
    drupal_write_record('fa_course', $course, 'facid');
  else
    drupal_write_record('fa_course', $course);
  drupal_set_message(t('Course @facid saved.', array('@facid' => $course->facid)));
  $form_state['redirect'] = "admin/people/courses/view/" . $course->facid;
}

function fa_course_delete($facid){
  $links = '<p>' . l(t("Back to courses"), "admin/people/courses") . '</p>';
  $output = render($links);
  $delete_form = drupal_get_form('fa_course_delete_form', $facid);
  $output .= render($delete_form);
  return $output;
}

function fa_course_delete_form($form, &$form_state, $facid) {
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

function fa_course_delete_form_validate($form, &$form_state) {
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

function fa_course_delete_form_submit($form, &$form_state) {
  $facid = $form_state['values']['facid'];

  db_delete('fa_course')
    ->condition('facid', $facid)
    ->execute();

  drupal_set_message(t('Course with id %facid has been deleted.', array('%facid' => $facid)));
  $form_state['redirect'] = "admin/people/courses";
}

function fa_course_trainer_handover($course){
  $links = '<p>' . l(t("Back"), "admin/people/courses/view/$course->facid") . '</p>';
  $output = render($links);
  $handover_form = drupal_get_form('fa_course_trainer_handover_form', $course);
  $output .= render($handover_form);
  return $output;
}

function fa_course_trainer_handover_form($form, &$form_state, $course){
  $form['course'] = array(
    '#type' => 'value',
    '#value' => $course,
  );
  return confirm_form(
    $form,
    t('Are you sure you want to handover the course @course_name '
      . 'to the trainer @trainer_email?',
      array('@course_name' => _fa_course_get_course_name($course->facid),
            '@trainer_email' => _fa_course_get_lead_trainer_info($course->facid)->mail)
      ),
    "admin/people/courses/view/" . $course->facid,
    t('This action cannot be undone.'),
    t('Handover'),
    t('Cancel')
  );
}

function fa_course_trainer_handover_form_validate($form, &$form_state) {
  $course = $form_state['values']['course'];

  // Check that course has the right values
  if (!isset($course->billing_information)){
    form_set_error('submit', t("No billing information given"));
  }
  if (!isset($course->lead_trainer_id)){
    form_set_error('submit', t("Lead trainer not set"));
  }

  // Get information about sessions
  $result = db_query("SELECT * FROM fa_course_session "
                . "WHERE facid = :facid ORDER BY session_start_datetime ASC",
                array(':facid' => $course->facid));
  $has_sessions = FALSE;
  foreach($result as $row){
    $has_sessions = TRUE;
    if (!isset($row->address)){
      form_set_error('submit', t("Session @session_id has no address",
                                array('@session_id' => $course->facid)));
    }
    if (!isset($row->material)){
      form_set_error('submit', t("Session @session_id has no material promises set",
                                array('@session_id' => $course->facid)));
    }
  }
  if (!$has_sessions){
    form_set_error('submit', t("Course has no sessions"));
  }

  // Get participant count
  $participant_count = db_query("SELECT COUNT(DISTINCT p.facpid) FROM fa_course_participant p "
                  . "WHERE p.facid = :facid",
                      array(':facid' => $course->facid)
                    )->fetchField();

  if ($participant_count == 0){
    form_set_error('submit', t("Course has no participants"));
  }

}

function fa_course_trainer_handover_form_submit($form, &$form_state) {
  $course = $form_state['values']['course'];
  $course->trainer_handover = REQUEST_TIME;

  // Send message to trainer
  // TODO

  drupal_write_record('fa_course', $course, 'facid');
  drupal_set_message(t('Course with id %facid has been handed over to the trainer.', array('%facid' => $course->facid)));
  $form_state['redirect'] = "admin/people/courses/view/" . $course->facid;
}

function fa_course_sales_handover($course){
  $output = "fa_course_sales_handover" . $course->facid;
  return $output;
}
