<?php

/**
 * @file
 * Course editing functions for Filosofian Akatemia Academians module.
 */



function fa_academian_overview() {
  return fa_academian_overview_period(0);
}

/**
 * Return a list of all courses.
 */
function fa_academian_overview_period($period) {
  $header = array(
    array('data' => t('Id'), 'field' => 'uid', 'sort' => 'desc'),
    array('data' => t('Email'), 'field' => 'name'),
    array('data' => t('Sessions'), 'field' => 'sessions'),
  );

  $result = db_select('users', 'u')
    ->fields('u', array('uid', 'name'))
    ->extend('TableSort')
    ->extend('PagerDefault')
    ->limit(50);
  $result->innerJoin('users_roles', 'ur', 'u.uid = ur.uid');
  $result->condition('ur.rid', 4);
  $result->leftJoin('fa_sessions_trainers', 'st', 'u.uid = st.uid');
  $result->addExpression('COUNT(DISTINCT st.facsid)', 'sessions');
  $result->leftJoin('fa_course_session', 's', 'st.facsid = s.facsid');
  $result->where('(st.facsid IS NULL) OR (YEAR(s.session_start_datetime) = YEAR(NOW()))');
  //$result->where('QUARTER(s.session_start_datetime) IN (3,4)');
  $result->distinct();
  $result->groupBy('u.uid');

  $result->orderByHeader($header);
  $result = $result->execute()->fetchAll();

  $rows = array();
  foreach ($result as $row) {
    $cells = array();
    $cells[] = $row->uid;
    $cells[] = $row->name;
    $cells[] = $row->sessions;
    $rows[] = $cells;
  }

  $output = theme('table', array('header' => $header, 'rows' => $rows));
  if (!$rows) {
    $output .= t('No academians found.');
  }
  else {
    $output .= theme('pager');
  }

  return $output;
}
