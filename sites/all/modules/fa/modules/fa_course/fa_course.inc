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
    array('data' => t('Product'), 'field' => 'product_name'),
    array('data' => t('Module'), 'field' => 'module'),
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
    ->fields('fac', array('facid', 'product_name', 'city', 'crm_link', 'trainer_handover', 'sales_handover', 'lead_sales_id', 'lead_trainer_id'))
    ->extend('TableSort')
    ->extend('PagerDefault')
    ->limit(50);
  $result->innerJoin('role', 'ro', 'ro.rid = fac.organization_id');
  $result->addField('ro', 'name', 'organization');
  $result->addField('ro', 'rid', 'organization_id');
  $result->leftJoin('role', 'rp', 'rp.rid = fac.module_id');
  $result->addField('rp', 'name', 'module');
  $result->addField('rp', 'rid', 'module_id');
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
    $course_name = _fa_course_format_course_name($row->product_name, $row->module_id, $row->organization_id, (isset($row->first_session) ? $row->first_session : NULL));
    $cells[] = l($course_name, "admin/people/courses/view/$row->facid");
    $cells[] = isset($row->first_session) ? format_date($row->first_session, 'custom', 'D d.m.Y H:i') : t('<i>Not set</i>');
    $cells[] = str_replace('#', '', $row->organization);
    $cells[] = $row->product_name;
    $cells[] = isset($row->module) ? str_replace('#', '', $row->module) : t('<i>Not set</i>');
    $cells[] = l($row->sales_first_name . " " . $row->sales_last_name, "user/" . $row->lead_sales_id);
    $cells[] = isset($row->trainer_first_name) ? l($row->trainer_first_name . " " . $row->trainer_last_name, "user/" . $row->lead_trainer_id) : t('<i>Not set</i>');
    $cells[] = $row->city;
    $cells[] = $row->course_role == NULL ? t('<i>Not set</i>') : $row->course_role;
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

  $academians = _fa_get_academians();
  if (!isset($course->lead_trainer_id))
    $academians[0] = t('-- unknown --');

  $course_roles = _fa_course_get_course_roles();

  // Not listed options
  $course_roles['organizations'][0] = t('--not listed--');
  // Uncomment if it should again be possible to add modules freely
  //$course_roles['modules'][0] = t('--not listed--');

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
  $form['course']['billing_sent'] = array(
      '#type' => 'textfield',
      '#title' => t('Billing information sent'),
      '#disabled' => TRUE,
      '#access' => isset($course->billing_sent),
      '#value' => isset($course->billing_sent) ?
                      format_date($course->billing_sent, 'custom', DATE_FORMAT) : NULL,
  );

  $form['course']['billing_sent_value'] = array(
      '#type' => 'textfield',
      '#title' => t('Value sent to billing'),
      '#disabled' => TRUE,
      '#access' => isset($course->price),
      '#value' => isset($course->price) ?
                      $course->price . ' ' . $course->currency : NULL,
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
  $form['course']['new_organization'] = array(
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

  // Product name
  $product_choices = preg_split( '/\r\n|\r|\n/', variable_get('fa_course_products', 'Kehityspolku'));
  if (isset($course->product_name) && !in_array($course->product_name, $product_choices)){
    array_push($product_choices, $course->product_name);
  }
  $form['course']['product'] = array(
      '#type'           => 'select',
      '#multiple'       => FALSE,
      '#disabled'       => $action == 'view',
      '#title'          => t('Course product'),
      '#default_value'  => isset($course->product_name) ? array_search($course->product_name, $product_choices) : 0,
      '#options'        => $product_choices,
  );

  // Module
  
  $form['course']['module'] = array(
      '#type' => 'fieldset',
      '#title' => t('Module'),
  );

  $form['course']['module']['module_check'] = array(
      '#type'           => 'checkbox',
      '#disabled'       => $action == 'view' || _fa_course_sessions_have_modules($course),
      '#title'          => t('This course contains substance from only one predefined module.'),
      '#default_value'  => isset($course->module_id) ? TRUE : FALSE,
  );
  
  $form['course']['module']['module_select'] = array(
      '#type'           => 'select',
      '#multiple'       => FALSE,
      '#title'          => t('Select Module'),
      '#disabled'       => $action == 'view',
      '#options'        => $course_roles['modules'],
      '#default_value'  => isset($course->module_id) ? $course->module_id : NULL,
      '#description'    => t('Select the module.'),
      '#states' => array(
          'visible' => array(
              ':input[name=module_check]' => array('checked' => TRUE),
          ),
      ),
	  
  );
  $form['course']['module']['new_module'] = array(
      '#type'           => 'textfield',
      '#title'          => t('Custom module'),
      '#disabled'       => $action == 'view',
      '#description'    => t('Type the name of the custom module here.'),
      '#states' => array(
          'visible' => array(   // action to take.
              ':input[name=module_check]' => array('checked' => TRUE),
              ':input[name=module_select]' => array('value' => 0),
          ),
          'required' => array(
              ':input[name=module_select]' => array('value' => 0),
          ),
      ),
  );

  $form['course']['city'] = array(
      '#type' => 'textfield',
      '#title' => t('City'),
      '#disabled'       => $action == 'view',
      '#default_value' => isset($course->city) ? $course->city: NULL,
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
      // Hide this from imported courses
      '#access' => !(isset($course->trainer_handover) && isset($course->sales_handover) &&  !isset($course->billing_information)),
      '#disabled' => $action == 'view' || isset($course->billing_sent),
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

  if (isset($course->other_information)){
    $other_information = unserialize($course->other_information);
  }else{
    $other_information = array();
  }

  $participant_actions = array(1 => t("the lead sales' responsibility,"),
                               2 => t("the lead trainer's responsibility,"),
                               3 => t("not possible because the list of participants is not available before the first session."));

  $default_participant_action = 1;
  if (isset($other_information['participant'])){
    if ($other_information['participant'] == "N/A"){
      $default_participant_action = 3;
    }else{
      $default_participant_action = 2;
    }
  }

  $form['course']['participant_responsibility'] = array(
      '#type' => 'radios',
      '#title' => t('Importing participants to the course is '),
      '#disabled'       => $action == 'view',
      '#default_value' => $default_participant_action,
      '#options' => $participant_actions,
  );
  $form['course']['participant_information'] = array(
      '#type' => 'textarea',
      '#title' => t('Participant import information'),
      '#description' => t('How will the lead trainer get the participants? If the participants have not arrived in time, what should the lead trainer do? Who should he contact?'),
      '#disabled'       => $action == 'view',
      '#default_value' => isset($other_information['participant']) ? $other_information['participant'] : NULL,
      '#states' => array(
          'visible' => array(   // action to take.
              ':input[name=participant_responsibility]' => array('value' => 2),
          ),
          'required' => array(
              ':input[name=participant_responsibility]' => array('value' => 2),
          ),
      ),
  );

  $form['course']['other_information'] = array(
      '#type' => 'textarea',
      '#title' => t('Other information'),
      '#description' => t('Other information about the course'),
      '#disabled'       => $action == 'view',
      '#default_value' => isset($other_information['other']) ? $other_information['other'] : NULL,
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

function _fa_course_sessions_have_modules($course){
  if (isset($course->facid)){
	$result = db_query("SELECT s.module_id FROM fa_course_session s WHERE s.module_id IS NOT NULL AND s.facid = :facid", array(':facid' => $course->facid))->fetchField();
    if ($result != FALSE) return TRUE;
  }
  return FALSE;
}

function fa_course_edit_form_validate($form, &$form_state) {
  $organization_role = _fa_course_get_selected_role($form_state, 'organization');
  $module_role = _fa_course_get_selected_role($form_state, 'module');
  // Check that a role with given class name does not yet exist
  $roles = user_roles(TRUE);

  foreach($roles as $role_name){
    if (!isset($organization_role->rid) && $role_name == $organization_role->name){
      form_set_error('organization', t('Role name with given organization already exists'));
      return;
    }
    if (!isset($module_role->rid) && isset($module_role->name) && ($role_name == $module_role->name)){
      form_set_error('new_module', t('Role name with given module already exists'));
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

  // Set no participants value
  if ($form_state['values']['participant_responsibility'] == 3){
    $form_state['values']['participant_information'] = "N/A";
  }elseif ($form_state['values']['participant_responsibility'] == 2 && empty($form_state['values']['participant_information'])){
    form_set_error('participant_information', t('Participant import information is required if importing is the lead trainer\'s responsibility'));
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

  // Module role
  $module_role = _fa_course_get_selected_role($form_state, 'module');
  	
  if (isset($module_role->name)){
    if (!isset($module_role->rid)){
      // Module role needs to be saved
      user_role_save($module_role);
    }
    $course->module_id = $module_role->rid;
  }elseif (isset($course->module_id)){
  	$course->module_id = NULL;
  }

  $course->city = $form_state['values']['city'];
  $course->product_name = $form['course']['product']['#options'][$form_state['values']['product']];
  $course->crm_link = $form_state['values']['crm_link'];
  $course->billing_information = !empty($form_state['values']['billing_information']) ? $form_state['values']['billing_information'] : NULL;
  $course->lead_sales_id = $form_state['values']['lead_sales'];
  if ($form_state['values']['lead_trainer'] != 0){
    if (!isset($course->lead_trainer_id) || ($course->lead_trainer_id != $form_state['values']['lead_trainer'])){
      $old_lead_trainer_id = FALSE;
      if (isset($course->lead_trainer_id)) $old_lead_trainer_id = $course->lead_trainer_id;
      $course->lead_trainer_id = $form_state['values']['lead_trainer'];
      // Go through every already created session and add the lead trainer to the trainers
      _fa_course_set_lead_trainer_to_sessions($course, $course->lead_trainer_id, $old_lead_trainer_id);
    }
  }

  // Other information holds two values
  $other_information = array();
  if (!empty($form_state['values']['other_information'])){
    $other_information['other'] = $form_state['values']['other_information'];
  }
  if (!empty($form_state['values']['participant_information'])){
    $other_information['participant'] = $form_state['values']['participant_information'];
  }

  $course->other_information = !empty($other_information) ? serialize($other_information) : NULL;

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

function _fa_course_set_lead_trainer_to_sessions($course, $current_lead_trainer_id, $old_lead_trainer_id){
  if (!isset($course->facid)) return;
  $result = db_query('SELECT s.facsid FROM fa_course_session s WHERE s.facid = :facid', array(':facid' => $course->facid));

  foreach ($result as $record) {
    _fa_course_set_lead_trainer_to_session($record->facsid, $course->lead_trainer_id, $old_lead_trainer_id);
  }
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

  $other_information = array();
  if (isset($course->other_information)){
    $other_information = unserialize($course->other_information);
  }

  if (($participant_count == 0) && (!isset($other_information['participant']))){
    form_set_error('submit', t("Course has no participants"));
  }
}

function fa_course_trainer_handover_form_submit($form, &$form_state) {
  $course = $form_state['values']['course'];
  $course->trainer_handover = REQUEST_TIME;

  module_load_include('inc', 'fa_course', 'fa_course.email');

  if (fa_course_send_trainer_handover($course) == FALSE){
    drupal_set_message(t('Error sending email to trainer.'));
    return;
  }

  drupal_write_record('fa_course', $course, 'facid');
  drupal_set_message(t('Course with id %facid has been handed over to the trainer.', array('%facid' => $course->facid)));
  $form_state['redirect'] = "admin/people/courses/view/" . $course->facid;
}

function fa_course_sales_handover($course){
  $links = '<p>' . l(t("Back"), "admin/people/courses/view/$course->facid") . '</p>';
  $output = render($links);
  $handover_form = drupal_get_form('fa_course_sales_handover_form', $course);
  $output .= render($handover_form);
  return $output;
}

function fa_course_sales_handover_form($form, &$form_state, $course){
  $form['course'] = array(
    '#type' => 'value',
    '#value' => $course,
  );
  return confirm_form(
    $form,
    t('Are you sure you want to handover the course @course_name '
      . 'back to sales?',
      array('@course_name' => _fa_course_get_course_name($course->facid))
      ),
    "admin/people/courses/view/" . $course->facid,
    t('This action cannot be undone.'),
    t('Handover'),
    t('Cancel')
  );
}

function fa_course_sales_handover_form_validate($form, &$form_state) {
  $course = $form_state['values']['course'];

  // Check that course has the right values
  if (!isset($course->trainer_evaluation)){
    form_set_error('submit', t("Trainer evaluation not given"));
  }

  // Sessions have to have all ended
  $unfinished_sessions_count = db_query("SELECT COUNT(DISTINCT s.facsid) FROM fa_course_session s "
                  . "WHERE s.facid = :facid AND s.session_end_datetime > :now",
                      array(':facid' => $course->facid, ':now' => REQUEST_TIME)
                    )->fetchField();

  if ($unfinished_sessions_count > 0){
    form_set_error('submit', t("All sessions are not finished yet"));
  }

}

function fa_course_sales_handover_form_submit($form, &$form_state) {
  $course = $form_state['values']['course'];
  $course->sales_handover = REQUEST_TIME;

  module_load_include('inc', 'fa_course', 'fa_course.email');

  if (fa_course_send_sales_handover($course) == FALSE){
    drupal_set_message(t('Error sending email to sales.'));
    return;
  }

  drupal_write_record('fa_course', $course, 'facid');
  drupal_set_message(t('Course with id %facid has been handed back over to sales.', array('%facid' => $course->facid)));
  $form_state['redirect'] = "admin/people/courses/view/" . $course->facid;
}
