<?php

/**
 * @file
 * Capsulecrm functions for Filosofian Akatemia Course module.
 */

define("OPPORTUNITY_URL_PREFIX", 'https://filosofianakatemia.capsulecrm.com/');

function fa_course_validate_crm_opportunity_url($url){
  return (strlen($url) > strlen(OPPORTUNITY_URL_PREFIX))
         && (!strncmp($url, OPPORTUNITY_URL_PREFIX, strlen(OPPORTUNITY_URL_PREFIX))
         && (strpos($url, '/opportunity/') !== FALSE));
}

function fa_course_get_crm_opportunity($url) {
  if (fa_course_validate_crm_opportunity_url($url) == FALSE){
    drupal_set_message(t("Invalid CapsuleCRM opportunity URL"), 'error');
    return NULL;
  }
  $username = variable_get('fa_course_capsulecrm_api_authentication_token', NULL);
  if ($username == NULL){
    drupal_set_message(t("CapsuleCRM API authentication token not set, exiting"), 'error');
    return NULL;
  }

  $opportunity_id = _fa_course_get_crm_opportunity_id($url);
  $result = drupal_http_request('https://' . $username . ':x@filosofianakatemia.capsulecrm.com/api/opportunity/' . $opportunity_id);

  if ($result->code == 200){
    $xml = simplexml_load_string($result->data);
    return $xml;
  }else{
    if (isset($result->data)){
      $xml = simplexml_load_string($result->data);
      drupal_set_message(t("Invalid response code @code from CapsuleCRM with message '@message', exiting", array('@code' => $result->code, '@message' => $xml->message)), 'error');
    }else{
      drupal_set_message(t("Invalid response code @code from CapsuleCRM, exiting", array('@code' => $result->code)), 'error');
    }
  }
  return NULL;
}

function _fa_course_get_crm_opportunity_id($url) {
  $keys = parse_url($url); // parse the url
  $path = explode("/", $keys['path']); // splitting the path
  return end($path); // get the value of the last element
}
