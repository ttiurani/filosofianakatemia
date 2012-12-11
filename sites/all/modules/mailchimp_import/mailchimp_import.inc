<?php
// $Id$

/**
 * Implementation of hook_admin_settings().
 */
function mailchimp_import_admin_settings() {

  // Only show the config options & import button if account info has been configured
  // for MailChimp module.  If not, show message with link to MailChimp settings page.
  $mailchimp_api_key = variable_get('mailchimp_api_key', FALSE);

  if ($mailchimp_api_key) {

    // Instantiate a MailChimp API object.
    $q = mailchimp_get_api_object();

    // Get all lists in the configured MailChimp account.
    $lists = $q->lists();

    // Build the options array.
    $options = array();

    foreach ($lists['data'] as $list) {
      $options[$list['id']] = $list['name'];
    }

    $form['mailchimp_import_lists'] = array(
      '#type'           => 'select',
      '#multiple'       => 1,
      '#title'          => t('Lists'),
      '#options'        => $options,
      '#default_value'  => variable_get('mailchimp_import_lists', array()),
      '#description'    => t('Select the lists you want to import campaigns from.'),
    );

    $status_options = array(
      'save' => 'Saved',
      'paused' => 'Paused',
      'schedule' => 'Scheduled',
      'sending' => 'Sending',
      'sent' => 'Sent'
    );

    $form['mailchimp_import_campaign_status'] = array(
      '#type'           => 'select',
      '#multiple'       => 1,
      '#title'          => t('Campaign status'),
      '#options'        => $status_options,
      '#default_value'  => variable_get('mailchimp_import_campaign_status', array('sent')),
      '#description'    => t('Select the statuses of the campaigns you want to import.'),
    );

    $form['mailchimp_import_nodetype'] = array(
      '#type'           => 'select',
      '#multiple'       => FALSE,
      '#title'          => t('Node type of imported campaigns'),
      '#options'        => node_type_get_names(),
      '#default_value'  => variable_get('mailchimp_import_nodetype', ''),
      '#description'    => t('Select the node type to create the imported campaigns as.'),
    );

    if (module_exists('i18n')){
      $form['mailchimp_import_language'] = i18n_element_language_select(
          variable_get('mailchimp_import_language', LANGUAGE_NONE));
      $form['mailchimp_import_language']['#description'] = t('Select the language for the created node.');
    }

    $form['mailchimp_import_menu'] = array(
        '#type'           => 'checkbox',
        '#title'          => t('Add main menu link with default parent.'),
        '#description'    => t('If checked, MailChimp Import will add a menu link using the default parent given to the node type, or if no default is given, set the link to the root of the main menu.'),
        '#default_value'  => variable_get('mailchimp_import_menu', FALSE)
    );

    // Input formats.
    $formats = filter_formats();

    // Only if multiple input formats are available, build select box.
    if (count($formats) > 1) {

      $format_options = array();
      foreach ($formats as $format) {
        $format_options[$format->format] = $format->name;
      }

      $form['mailchimp_import_input_format'] = array(
        '#type'           => 'select',
        '#multiple'       => FALSE,
        '#title'          => t('Input format of imported campaigns'),
        '#options'        => $format_options,
        '#default_value'  => variable_get('mailchimp_import_input_format', ''),
        '#description'    => t('Select the input format for the content of imported campaigns.'),
      );
    }

    $form['mailchimp_import_cron'] = array(
      '#type'           => 'checkbox',
      '#title'          => t('Check for campaigns to import on every cron run.'),
      '#description'    => t('If checked, MailChimp Import will check for campaigns to import from your MailChimp
                             account every time cron runs.'),
      '#default_value'  => variable_get('mailchimp_import_cron', TRUE)
    );

    $form['mailchimp_import_extract_title'] = array(
        '#type'           => 'checkbox',
        '#title'          => t('Extract node title from content'),
        '#description'    => t('If checked, node title will be extracted and then removed from the
            content.'),
        '#default_value'  => variable_get('mailchimp_import_extract_title', FALSE)
    );

    $form['mailchimp_import_title_id'] = array(
        '#type'           => 'textfield',
        '#title'          => t('Title Id'),
        '#description'    => t('Id of the title portion in the campaign. Typically id
            of a div.'),
        '#default_value'  => variable_get('mailchimp_import_title_id', 'mailchimp-import-title'),
        '#states' => array(
            'visible' => array(
                ':input[name=mailchimp_import_extract_title]' => array('checked' => TRUE),
            ),
        ),
    );

    $form['mailchimp_import_only_selected'] = array(
        '#type'           => 'checkbox',
        '#title'          => t('Import only selected portion of campaign'),
        '#description'    => t('If checked, MailChimp Import will import only selected portion
            from the campaign.'),
        '#default_value'  => variable_get('mailchimp_import_only_selected', FALSE)
    );

    $form['mailchimp_import_content_id'] = array(
        '#type'           => 'textfield',
        '#title'          => t('Content Id'),
        '#description'    => t('Id of the content portion in the campaign. Typically id of a div.'),
        '#default_value'  => variable_get('mailchimp_import_content_id', 'mailchimp-import'),
        '#states' => array(
            'visible' => array(
                ':input[name=mailchimp_import_only_selected]' => array('checked' => TRUE),
            ),
        ),
    );

    $form['mailchimp_import_cleanup_html'] = array(
      '#type'           => 'fieldset',
      '#title'          => t('HTML cleanup options'),
      '#collapsible' => TRUE,
      '#collapsed' => TRUE,
    );

    // Get the HTML cleanup options.
    $cleanup_options = variable_get('mailchimp_import_cleanup_html', array());

    $form['mailchimp_import_cleanup_html']['mailchimp_import_strip_style'] = array(
      '#type'           => 'checkbox',
      '#title'          => t('Strip out embedded CSS in &lt;style&gt; tags and style attributes.'),
      '#description'    => t('If checked, MailChimp Import will strip out any &lt;style&gt; tags and the code
        between them from the campaign content on import, and also style attributes in other tags. This is useful,
        because otherwise embedded CSS from the campaign can mess up page styles.'),
      '#default_value'  => isset($cleanup_options['mailchimp_import_strip_style']) ?
        $cleanup_options['mailchimp_import_strip_style'] : TRUE,
    );

    $form['mailchimp_import_cleanup_html']['mailchimp_import_strip_script'] = array(
      '#type'           => 'checkbox',
      '#title'          => t('Strip out Javascript in &lt;script&gt; tags.'),
      '#description'    => t('If checked, MailChimp Import strip out any &lt;script&gt; tags and the code
        between them from the campaign content on import.'),
      '#default_value'  => isset($cleanup_options['mailchimp_import_strip_script']) ?
        $cleanup_options['mailchimp_import_strip_script'] : TRUE,
    );

    // Settings for stripping out certain tags and the content between them.
    $invisible_tags = array('!DOCTYPE', 'head', 'object', 'embed', 'applet', 'noframes', 'noscript', 'noembed');
    foreach ($invisible_tags as $tag) {
      $form['mailchimp_import_cleanup_html']['mailchimp_import_strip_'.$tag] = array(
        '#type'           => 'checkbox',
        '#title'          => 'Strip ' . $tag . '  tags.',
        '#description'    => 'If checked, MailChimp Import will strip out any &lt;' . $tag . '&gt; tags and any
        code or text inside them from the campaign content on import.',
        '#default_value'  => isset($cleanup_options['mailchimp_import_strip_'.$tag]) ?
          $cleanup_options['mailchimp_import_strip_'.$tag] : TRUE,
      );
    }

    $form['mailchimp_import_campaigns'] = array(
      '#type' => 'fieldset',
      '#title' => t('Import campaigns'),
      '#description' => t('When you press this button, your MailChimp campaigns will be imported as Drupal nodes.'),
    );

    $form['mailchimp_import_campaigns']['import'] = array(
      '#type' => 'submit',
      '#value' => t('Import campaigns'),
      '#submit' => array('mailchimp_import_get_campaigns'),
    );

    return system_settings_form($form);

  }
  else {
    // MailChimp account info has not yet been configured.  Provide instructions and a config link with a
    // destination query which will bring us back here.
    $form['message'] = array(
      '#value' => t('Before you can use MailChimp Import, you must first <a href="@config_path">configure
                    MailChimp module</a> with your MailChimp API key.',
                    array('@config_path' => url('admin/config/services/mailchimp', array('query' => array(
                      'destination' => 'admin/config/services/mailchimp/import'))))),
    );
    return $form;
  }
}


/**
 * Submit handler for the Import Campaigns button; imports MailChimp messages according to
 * the configured MailChimp Import settings.  This is the Main function of this module,
 * that does all the work of importing campaigns and saving them as nodes.
 */
function mailchimp_import_get_campaigns($form = array(), &$form_state = array()) {

  // Instantiate a MailChimp API object.
  if ($q = mailchimp_get_api_object()) {

    // Process variables.  Try to use the values from the form first; if not set, use the saved variables.
    $lists         = count($form['mailchimp_import_lists']['#value']) ? $form['mailchimp_import_lists']['#value'] :
                      variable_get('mailchimp_import_lists', array());
    $status        = count($form['mailchimp_import_campaign_status']['#value']) ? $form['mailchimp_import_campaign_status']['#value'] :
                      variable_get('mailchimp_import_campaign_status', array('sent' => 'sent'));
    $input_format  = is_numeric($form['mailchimp_import_input_format']['#value']) ?
                      $form['mailchimp_import_input_format']['#value'] :
                      variable_get('mailchimp_import_input_format', 1);
    $nodetype      = $form['mailchimp_import_nodetype']['#value'] ? $form['mailchimp_import_nodetype']['#value'] :
                      variable_get('mailchimp_import_nodetype', 1);
    $language      = $form['mailchimp_import_language']['#value'] ? $form['mailchimp_import_language']['#value'] :
                      variable_get('mailchimp_import_language', LANGUAGE_NONE);
    $add_menu_link = $form['mailchimp_import_menu']['#value'] ? $form['mailchimp_import_menu']['#value'] :
                      variable_get('mailchimp_import_menu', FALSE);

    if (count($lists)) {

      // Get an array of campaigns we've already imported.
      $already_imported_campaigns = mailchimp_import_get_imported_campaigns();

      // Instantiate a counter, so we can report how many campaigns were imported at the end.
      $counter = 0;

      // Loop through lists.
      foreach ($lists as $list) {
        // Get all campaigns in the current list.
        $campaigns = $q->campaigns(array('list_id' => $list));

        if (is_array($campaigns['data']) && count($campaigns['data'])) {

          // Loop through the campaigns we imported from MailChimp.
          foreach ($campaigns['data'] as $campaignKey => $campaign) {

            // Only import campaigns that have the right status.
            if (array_key_exists($campaign['status'], variable_get('mailchimp_import_campaign_status', array('sent' => 'Sent')))) {

              // Check if this campaign has already been imported.
              if (!in_array($campaign['id'], $already_imported_campaigns)) {

                // Get this campaign's HTML content.
                $campaign_content = $q->campaignContent($campaign['id']);
                $campaign_html = $campaign_content['html'];

                // Set default value for title
                $node_title = $campaign['subject'];

                // Check if title should be extracted
                if (variable_get('mailchimp_import_extract_title', FALSE)){
                  $title = _mailchimp_import_extract_title($campaign_html,
                      variable_get('mailchimp_import_title_id', 'mailchimp-import-title'));
                  if ($title != NULL){
                    $node_title = $title;
                  }
                }

                // Check if only selected divs should be included
                if (variable_get('mailchimp_import_only_selected', FALSE)){
                  $campaign_html = _mailchimp_import_get_selected($campaign_html,
                      variable_get('mailchimp_import_content_id', 'mailchimp-import'));
                }

                if ($campaign_html != NULL){

                  // Strip out the HTML.
                  $campaign_html = mailchimp_import_strip_html($campaign_html, $input_format);

                  // Get the user object to assign as the author.
                  global $user;

                  // Create a node for this campaign.
                  $node = new stdClass();
                  $node->type = $nodetype;
                  $node->uid = $user->uid;
                  $node->title = $node_title;
                  $node->language = $language;
                  $node->body[LANGUAGE_NONE][0]['value']   = $campaign_html;
                  $node->body[LANGUAGE_NONE][0]['format']  = $input_format;;
                  $node->created = strtotime($campaign['send_time']); // The created date of the node will be set to the sent date of the campaign.
                  if ($add_menu_link){
                    $node->menu =_get_main_menu_link($node);
                  }

                  node_save($node);

                  // If there's a nid now, then we saved it.
                  if ($node->nid) {
                    // Insert a record into the MailChimp Import table.
                    db_query('INSERT INTO {mailchimp_import} (cid, nid) VALUES (:cid, :nid)',
                        array(':cid' => $campaign['id'], ':nid' => $node->nid));
                    $counter++;
                    drupal_set_message(t('Imported campaign <a href="@imported_campaign_path">%imported_campaign_title</a>.', array('@imported_campaign_path' => url('node/' . $node->nid), '%imported_campaign_title' => $node_title)));
                  }
                }
              }
            }
          }
        }
      }
      // Report how many campaigns were imported.
      if ($counter == 0) {
        $message = t('No campaigns were imported from MailChimp.');
      }
      else if ($counter == 1) {
        $message = t('1 campaign was imported from MailChimp and saved as a %nodetype node.', array('%nodetype' => $nodetype));
      }
      else {
        $message = t('%counter campaigns were imported from MailChimp and saved as %nodetype nodes.', array('%counter' => $counter, '%nodetype' => $nodetype));
      }
      drupal_set_message($message);
    }
    else {
      // If we're here, no lists were chosen.  Tell the user to pick some lists.
      drupal_set_message(t('You need to first select at least one list from which to import.'));
    }
  }
  else {
    drupal_set_message(t('Couldn\'t connect to MailChimp!'), 'warning');
  }
}

/**
 * Helper function. Adds menu link.
 */
function _get_main_menu_link($node){
  $parent = variable_get('menu_parent_' . $node->type, 'main-menu:0');
  $parent_array = explode(':', $parent);
  $plid = $parent_array[1];
  return array(
      'enabled' => 1,
      'mlid' => 0,
      'module' => 'menu',
      'hidden' => 0,
      'has_children' => 0,
      'customized' => 0,
      'options' => array(),
      'expanded' => 0,
      'parent_depth_limit' => 8,
      'link_title' => $node->title,
      'description' => '',
      'parent' => $parent,
      'plid' => $plid,
      'weight' => 0,
      'menu_name' => 'main-menu'
   );
}

/**
 * Helper function.  Get an array of campaign IDs of the campaigns we've already imported.
 */
function mailchimp_import_get_imported_campaigns() {
  $campaigns = array();
  $result = db_query('SELECT * FROM {mailchimp_import};');

  foreach($result as $import){
    $campaigns[$import->nid] = $import->cid;
  }

  return $campaigns;
}


/**
 * Helper function. Get the contents of the div with id "mailchimp-import",
 * and optionally if found, strips out the div with id "mailchimp-import-heading"
 * and sets it to the heading variable.
 */
function _mailchimp_import_extract_title(&$html, $title_id) {

  // Need to create a DOM document of the HTML
  $dom=new domDocument;
  $dom->preserveWhiteSpace = false;
  $dom->validateOnParse = true;
  $dom->strictErrorChecking = false;
  @$dom->loadHTML($html);

  // Try to find the title
  $title_element = $dom->getElementById($title_id);
  if ($title_element == NULL){
    drupal_set_message(t('Could not find title element with id "' . $title_id . '"'), 'warning');
    return NULL;
  }

  $title = $title_element->nodeValue;
  $title_element->parentNode->removeChild($title_element);
  $html = $dom->saveXML();

  // Return string representation of the selected element
  return trim($title_element->nodeValue);
}

/**
 * Helper function. Returns the contents of an element with given id
 */
function _mailchimp_import_get_selected($html, $content_id) {

  // Need to create a DOM document of the HTML
  $dom=new domDocument;
  $dom->preserveWhiteSpace = false;
  $dom->validateOnParse = true;
  $dom->strictErrorChecking = false;
  @$dom->loadHTML($html);

  // Find the selected div
  $selected_element = $dom->getElementById($content_id);
  if ($selected_element == NULL){
    drupal_set_message(t('Could not find div with id "' . $content_id . '"'), 'warning');
    return NULL;
  }

  return $dom->saveXML($selected_element);
}


/**
 * Helper function.  Rip out tags such as STYLE and SCRIPT and the content between them.
 * Add line breaks around block-level tags to prevent word joining after tag removal.
 */
function mailchimp_import_strip_html($text, $input_format) {

  // Get the cleanup options.
  $cleanup_options = variable_get('mailchimp_import_cleanup_html', array());

  // Make an array of tags to strip along with their content.
  $strip_invisible = array();
  // Make an array of emptiness to replace these tags with.
  $replace_with = array();

  // Loop through possible tags to hide, adding the selected ones to the $strip_invisible array.
  $invisible_tags = array('head', 'object', 'embed', 'applet', 'noframes', 'noscript', 'noembed', 'style', 'script');
  foreach ($invisible_tags as $tag) {
    if (!isset($cleanup_options['mailchimp_import_strip_'.$tag]) || $cleanup_options['mailchimp_import_strip_'.$tag]) {
      $strip_invisible[] = '@<' . $tag . '[^>]*?>.*?</' . $tag . '>@siu';
      $replace_with[] = ' ';
      if ($tag == 'style'){
        // Need to remove style attribute as well
        $strip_invisible[] = '/(<[^>]+) style=".*?"/i';
        $replace_with[] = '$1';
      }
    }
  }

  // Process the !DOCTYPE tag specially.
  if (!isset($cleanup_options['mailchimp_import_strip_!DOCTYPE']) || $cleanup_options['mailchimp_import_strip_!DOCTYPE']) {
    $strip_invisible[] = '@<!DOCTYPE[^>]*?>.*?>@siu';
    $replace_with[] = ' ';
  }

  // Process the text.
  $text = preg_replace($strip_invisible, $replace_with, $text);

  return $text;
}