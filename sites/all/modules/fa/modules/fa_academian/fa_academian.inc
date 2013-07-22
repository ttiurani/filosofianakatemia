<?php

/**
 * @file
 * Course editing functions for Filosofian Akatemia Academians module.
 */



function fa_academian_overview() {
  return fa_academian_overview_period(1);
}

/**
 * Return a list of all courses.
 */
function fa_academian_overview_period($period_id) {
  $header = array(
    array('data' => t('UID'), 'field' => 'uid', 'sort' => 'desc'),
    array('data' => t('Email'), 'field' => 'name'),
    array('data' => t('Total Sessions'), 'field' => 'sessions'),
    array('data' => t('0.5 point sessions'), 'field' => 'halfpoints'),
    array('data' => t('1 point sessions'), 'field' => 'fullpoints'),
    array('data' => t('1.5 point sessions'), 'field' => 'oneandhalfpoints'),
    array('data' => t('2 point sessions'), 'field' => 'twopoints'),
    t('Total points'),
  );
  if ($period_id == 1)
    $header[] = t('Progression');

  $result = db_select('users', 'u')
    ->fields('u', array('uid', 'name'))
    ->extend('TableSort')
    ->extend('PagerDefault')
    ->limit(50);
  $result->innerJoin('users_roles', 'ur', 'u.uid = ur.uid');
  $result->condition('ur.rid', 4);
  $result->leftJoin('fa_sessions_trainers', 'st', 'u.uid = st.uid');
  $result->leftJoin('fa_course_session', 's',
           '((st.facsid = s.facsid) ' .
           'AND (YEAR(FROM_UNIXTIME(s.session_start_datetime)) = YEAR(DATE_SUB(NOW(), INTERVAL :day_interval DAY))) ' .
           'AND ((QUARTER(FROM_UNIXTIME(s.session_start_datetime)) IN (1,2) AND (QUARTER(DATE_SUB(NOW(), INTERVAL :day_interval DAY)) IN (1, 2)))' .
             'OR (QUARTER(FROM_UNIXTIME(s.session_start_datetime)) IN (3,4) AND (QUARTER(DATE_SUB(NOW(), INTERVAL :day_interval DAY)) IN (3, 4)))))',
             array(':day_interval' => ($period_id - 1) * 182));
  $result->addExpression('COUNT(DISTINCT s.facsid)', 'sessions');
  $result->addExpression('COUNT(CASE WHEN (s.session_end_datetime - s.session_start_datetime) <= :maxhalfpoint THEN 1 ELSE NULL END)',
                         'halfpoints',
                          array(':maxhalfpoint' => variable_get('fa_academian_half_max_minutes') * 60));
  $result->addExpression('COUNT(CASE WHEN ((s.session_end_datetime - s.session_start_datetime) > :maxhalfpoint) ' .
                                     'AND ((s.session_end_datetime - s.session_start_datetime) <= :maxpoint) ' .
                         'THEN 1 ELSE NULL END)',
                         'fullpoints',
                         array(':maxhalfpoint' => variable_get('fa_academian_half_max_minutes') * 60,
                               ':maxpoint' => variable_get('fa_academian_full_max_minutes') * 60));
  $result->addExpression('COUNT(CASE WHEN ((s.session_end_datetime - s.session_start_datetime) > :maxpoint) ' .
                                     'AND ((s.session_end_datetime - s.session_start_datetime) <= :maxoneandhalfpoint) ' .
                         'THEN 1 ELSE NULL END)',
                         'oneandhalfpoints',
                         array(':maxpoint' => variable_get('fa_academian_full_max_minutes') * 60,
                               ':maxoneandhalfpoint' => variable_get('fa_academian_oneandhalf_max_minutes') * 60));
  $result->addExpression('COUNT(CASE WHEN (s.session_end_datetime - s.session_start_datetime) > :maxoneandhalfpoint THEN 1 ELSE NULL END)',
                         'twopoints',
                         array(':maxoneandhalfpoint' => variable_get('fa_academian_oneandhalf_max_minutes') * 60));

  $result->distinct();
  $result->groupBy('u.uid');

  $result->orderByHeader($header);
  $result = $result->execute()->fetchAll();

  $rows = array();
  foreach ($result as $row) {
    $cells = array();
    $cells[] = $row->uid;
    $cells[] = l($row->name, "admin/people/academians/user/" . $row->uid);
    $cells[] = $row->sessions;
    $cells[] = $row->halfpoints;
    $cells[] = $row->fullpoints;
    $cells[] = $row->oneandhalfpoints;
    $cells[] = $row->twopoints;
    $total_points = $row->halfpoints * 0.5 + $row->fullpoints + $row->oneandhalfpoints * 1.5 + $row->twopoints * 2;

    $cells[] = $total_points;

    $trainer_points = variable_get('fa_academian_trainer_points_' . $row->uid, 0);

    if ($period_id == 1){
      if ($trainer_points == 0){
        $cells[] = 'N/A';
      }else{
        $cells[] = $total_points . "/" . $trainer_points . " = " . $total_points / $trainer_points * 100 . '%';
      }
    }
    $rows[] = $cells;
  }

  $output = "";
  $period = date('Y-m', REQUEST_TIME - ($period_id - 1) * 182 * 24 * 60 * 60);
  $month = substr($period, 5, 2);
  $season = t("spring");
  if ($month > 6)
    $season = t("autumn");

  $heading = '<p><h2>' . t("Statistics for @season of @year", array('@season' => $season, '@year' =>  substr($period, 0, 4))) . '</h2></p>';
  $output .= render($heading);

  $links = '<p>' . l(t("Previous"), "admin/people/academians/" . ($period_id + 1) );
  if ($period_id > 0)
    $links .= " " . l(t("Next"), "admin/people/academians/" . ($period_id - 1) );
  $links .= '</p>';
  $output .= render($links);

  $output .= theme('table', array('header' => $header, 'rows' => $rows));
  if (!$rows) {
    $output .= t('No academians found.');
  }
  else {
    $output .= theme('pager');
  }

  return $output;
}

/**
 * Return info for the single user.
 */
function fa_academian_user($uid) {

  module_load_include('inc', 'fa_course', 'fa_course.sessions');

  $header = array(
      t('Course'),
      array('data' => t('Session Id'), 'field' => 'facsid'),
      t('Session type'),
      array('data' => t('Start time'), 'field' => 'session_start_datetime', 'sort' => 'asc'),
      t('End Time'),
      t('Address'),
      t('Material'),
  );

  $session_type = _fa_course_get_session_types();
  $result = db_select('fa_course_session', 's')
    ->fields('s', array('facid', 'facsid', 'session_type', 'session_start_datetime', 'session_end_datetime', 'address', 'material', 'assistant_email'))
    ->extend('TableSort')
    ->extend('PagerDefault');
  $result->innerJoin('fa_sessions_trainers', 'st', 's.facsid = st.facsid');
  $result->condition('st.uid', $uid);
  $result->where('s.session_start_datetime > UNIX_TIMESTAMP(NOW())');
  $result->distinct();
  $result->groupBy('s.facsid');
  $result->orderByHeader($header);
  $result = $result->execute()->fetchAll();

  $rows = array();
  foreach ($result as $row) {
    $cells = array();
    $cells[] = l(_fa_course_get_course_name($row->facid), "admin/people/courses/sessions/" . $row->facid . "/edit/$row->facsid");
    $cells[] = $row->facsid;
    $cells[] = $session_type[$row->session_type];
    $cells[] = format_date($row->session_start_datetime, 'custom', 'D d.m.Y H:i');
    $cells[] = format_date($row->session_end_datetime, 'custom', 'D d.m.Y H:i');
    $cells[] = $row->address;
    $cells[] = $row->material;
    $rows[] = $cells;
  }

  $academians = _fa_get_academians();
  $title = '<p><strong>Future sessions for ' . $academians[$uid] . '</strong><br/>'
            . l(t("Back"), "admin/people/academians") . '</p>';
  $output = render($title);

  $output .= theme('table', array('header' => $header, 'rows' => $rows));

  if (!$rows) {
    $output .= t('No future sessions found.');
  }
  else {
    $output .= theme('pager');
  }
  return $output;

}
