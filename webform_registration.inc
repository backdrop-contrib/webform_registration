<?php
/**
 * @file
 * Webform Registration Include File.
 *
 * This file holds all the end-user-facing functionality.
 * This file is only here to make the module easier to read.
 */

/**
 * Implements hook_form_alter().
 * Alters instances of registration enabled webforms.
 */
function webform_registration_inc_form_alter(&$form, &$form_state, $form_id) {
  global $user;

  //Set user object
  $form['#user'] = $user;

  //Load the webform settings
  $settings = $form['#node']->webform_registration;

  if (!$settings['enabled']) {
    return;
  }

  $formdata = _webform_registration_prepare_formdata($form, $form_state);
  $fields = _webform_registration_get_fields($formdata, $settings);

  // Username field handling.
  if (isset($fields['name']['field'])) {
    $fields['name']['field']['#required'] = TRUE;
  }

  // Password field handling.
  if ($settings['pass'] != '<auto>') {
    $fields['pass']['field']['#type'] = 'password';
    $fields['pass']['field']['#required'] = TRUE;
  }

  // Password confirmation field handling
  if ($settings['pass_confirm'] != '<none>') {
    $fields['pass_confirm']['field']['#type'] = 'password';
  }

  // Email field handling
  if (isset($fields['mail']['field'])) {
    $fields['mail']['field']['#required'] = TRUE;
  }

  // The user is logged in. QUICK! HIDE!
  if (user_is_logged_in()) {
    // Hide everything but populate the fields in case webform needs them for
    // something, e.g. Connecting with a CRM or such
    //Load User Data
    $user_data = user_load($user->uid);

    if (is_array($settings['account_fields'])) {
      foreach ($settings['account_fields'] as $account_field => $field) {
        if (isset($user_data->{$account_field}['und'][0]['value'])) {
          $fields[$account_field]['field']['#attributes'] = array('readonly' => 'readonly');
          $fields[$account_field]['field']['#value'] = $user_data->{$account_field}['und'][0]['value'];
        }
      }
    }

    $fields['name']['field']['#attributes'] = array('readonly' => 'readonly');
    $fields['name']['field']['#value'] = $user->name;

    $fields['pass']['field']['#type'] = 'value';
    $fields['pass']['field']['#value'] = '';

    $fields['pass_confirm']['field']['#type'] = 'value';
    $fields['pass_confirm']['field']['#value'] = '';

    $fields['mail']['field']['#attributes'] = array('readonly' => 'readonly');
    $fields['mail']['field']['#value'] = $user->mail;

    $fields['mail_confirm']['field']['#attributes'] = array('readonly' => 'readonly');
    $fields['mail_confirm']['field']['#value'] = $user->mail;
    $form['#submit'][] = 'webform_registration_upgrade_submit';
  }
  else {
    $form['#validate'][] = 'webform_registration_validate';
    $form['#submit'][] = 'webform_registration_submit';
  }
  //$formkeys = _webform_registration_get_formkeys($form);
  //_webform_registration_set_fields($formdata, $fields, $formkeys);
}

/**
 * Webform Registration Validate Function.
 */
function webform_registration_validate($form, &$form_state) {

  //Load the webform settings
  $settings = $form['#node']->webform_registration;
  $account = $form['#user'];

  if (isset($form_state['values']['submitted'])) {
    $formdata = _webform_registration_prepare_formdata($form, $form_state);
  }
  $fields = _webform_registration_get_fields($formdata, $settings);
  
  // Validate new or changing username.
  if (isset($fields['name'])) {
      if ($error = user_validate_name($fields['name']['value'])) {
      form_set_error($fields['name']['cid'], $error);
    }
    elseif ((bool) db_select('users')->fields('users', array('uid'))->condition('uid', $account->uid, '<>')->condition('name', db_like($fields['name']['value']), 'LIKE')->range(0, 1)->execute()->fetchField()) {
      form_set_error($fields['name']['cid'], t('The name %name is already taken.', array('%name' => $fields['name']['value'])));
    }
  }

  // Validate the e-mail address, and check if it is taken by an existing user.
  if (!(valid_email_address($fields['mail']['value']))) {
    form_set_error($fields['mail']['cid'], t('Please enter a valid email address.'));
  }
  elseif ((bool) db_select('users')->fields('users', array('uid'))->condition('uid', $account->uid, '<>')->condition('mail', db_like($fields['mail']['value']), 'LIKE')->range(0, 1)->execute()->fetchField()) {
    // Format error message dependent on whether the user is logged in or not.
    if (user_is_logged_in()) {
      form_set_error($fields['mail']['cid'], t('The e-mail address %email is already taken.', array('%email' => $fields['mail']['value'])));
    }
    else {
      form_set_error($fields['mail']['cid'], t('The e-mail address %email is already registered. <a href="@password">Have you forgotten your password?</a>', array('%email' => $fields['mail']['value'], '@password' => url('user/password'))));
    }
  }

  // Email Confirmation
  if ($settings['mail_confirm'] != '<none>' && $fields['mail']['value'] != $fields['mail_confirm']['value']) {
    form_set_error($fields['mail_confirm']['cid'], t('Your email and confirmation addresses do not match'));
    return;
  }

  // Password Confirmation
  if ($settings['pass_confirm'] != '<none>' && $fields['pass']['value'] != $fields['pass_confirm']['value']) {
    form_set_error($fields['pass_confirm']['cid'], t('Your password and confirmation values do not match'));
    return;
  }


}

/**
 * Webform Registration Submit Handler - existing user
 */
function webform_registration_upgrade_submit($form, &$form_state) {
  //Load the webform settings
  $settings = $form['#node']->webform_registration;

  global $user;
  $account = user_load($user->uid);
  $roles = user_roles(TRUE);
  $all_roles = $account->roles;
  if (isset($settings['roles']) && is_array($settings['roles'])) {
    foreach ($settings['roles'] as $rolename => $rolelabel) {
      $all_roles[$rolename] = $roles[$rolename];
    }
  }
  user_save($account, array('roles' => $all_roles));
}

/**
 * Webform Registration Submit Handler - new user
 */
function webform_registration_submit($form, &$form_state) {

  //Load the webform settings
  $settings = $form['#node']->webform_registration;
  
  // Retrieve the values and/or form references
  $formdata = _webform_registration_prepare_formdata($form, $form_state);
  $fields = _webform_registration_get_fields($formdata, $settings);

  $formkeys = _webform_registration_get_formkeys($form);
  $opt_in_cid = array_search('opt_in', $formkeys);
  if ($settings['opt_in'] !== '<none>' && $settings['opt_in_value'] != $formdata[$opt_in_cid]) {
    // Has user has opted out of registration? If so, stop processing.
    return;
  }

  // Make sure it's not a multi-page form that is not yet complete.
  $submit_op = !empty($form['actions']['submit']['#value']) ? $form['actions']['submit']['#value'] : t('Submit');
  if ($form_state['values']['op'] == $submit_op) {
    // Submission is complete so create the user.
    
    watchdog('webform_registration', 'Webform Registration: Registration Triggered');
    
    $admin = user_access('administer users');
    
    $user_register_info = array(
      'values' => array(
        'mail' => $fields['mail']['value'],
        'name' => ($fields['name']['value'] != '' ? $fields['name']['value'] : $fields['mail']['value']),
        'notify' => 0,
        'status' => $settings['status'],
        'pass' => ($fields['pass']['cid'] != '<auto>' ? $fields['pass']['value'] : user_password()),
        'roles' => $settings['roles'],
        'administer_users' => $admin,
      ),
      'buttons' => array(),
    );
  
    //Add use field instances
    $user_field_info_instances = field_info_instances('user', 'user');
    foreach ($user_field_info_instances as $field_name => $instance) {
      if (!in_array($fields[$field_name], _webform_registration_get_special_cases())) {
        $user_register_info['values'][$field_name]['und'][0]['value'] = $fields[$field_name];
      }
    }
    
    $form['#user'] = entity_create('user', array());
    user_register_submit($form, $user_register_info);
  
    // Update the main submission info.
    db_update('webform_submissions')
      ->fields(array(
        'uid' => $user_register_info['values']['uid'],
      ))
      ->condition('sid', $form_state['values']['details']['sid'])
      ->execute();
      
    // Redact the passwords.
    $password_field_cid = $confirm_password_field_cid = NULL;
    $password_field_key = $form['#node']->webform_registration['pass'];
    $confirm_password_field_key = $form['#node']->webform_registration['pass_confirm'];
  
    $components = $form['#node']->webform['components'];
  
    foreach($components as $each_component) {
      if ($each_component['form_key'] == $password_field_key) {
        $password_field_cid = $each_component['cid'];
      }
      elseif ($each_component['form_key'] == $confirm_password_field_key) {
        $confirm_password_field_cid = $each_component['cid'];
      }
    }
  
    db_update('webform_submitted_data')
      ->fields(array(
        'data' => '[REDACTED]',
      ))
      ->condition('sid', $form_state['values']['details']['sid'])
      ->condition('cid', array($password_field_cid, $confirm_password_field_cid), 'IN')
      ->execute();
  }

}

/**
 * Helper function that prepares the formdata array, including data from earlier
 * webform pages. Components will be keyed on the component id.
 */
function _webform_registration_prepare_formdata($form, &$form_state) {
  $formkeys = _webform_registration_get_formkeys($form);
  $formdata = array();
  if (!empty($form_state['values']) && !empty($form_state['values']['submitted'])) {
    // Add data from 'values' array if it exists. This is already keyed by 'cid'.
    $formdata += $form_state['values']['submitted'];
  }
  if (!empty($form_state['input']['submitted'])) {
    // Add newly submitted data, but rekey it from $form_key to $cid.
    $formdata += _webform_registration_rekey($form_state['input']['submitted'], $formkeys);
  }
  if (!empty($form_state['storage']['submitted'])) {
    // Also add data from previous pages in case of multipage forms. This 
    // is already keyed by 'cid'.
    $formdata += $form_state['storage']['submitted'];
  }
  return $formdata;
}

/**
 * Helper function that converts 'name' keys to 'cid' (component id) keys.
 */
function _webform_registration_rekey($original, $formkeys) {
  $formdata_additions = array();
  foreach($original as $name => $value) {
    if (is_array($value)) {
      // This is a nested/parent field (e.g. fieldset).
        $child_additions = _webform_registration_rekey($value, $formkeys);
        $formdata_additions += $child_additions;
    }
    elseif (!is_numeric($name)) {
      // Not a numeric key. Assume it is a 'name' key.
      $cid = array_search($name, $formkeys);
      if ($cid) {
        $formdata_additions[$cid] = $value;
      }
    }
    else {
      // This is numeric. This is not expected, but assume this is already keyed to cid.
      $formdata_additions[$name] = $value;
    }
  }
  return $formdata_additions;
}

/**
 * Helper function that retrieves the values to be used with the user
 * registration. This is front-end and end-user.
 */
function _webform_registration_get_formkeys($form) {
  $formkeys = array();
  foreach ($form['#node']->webform['components'] as $component) {
    $formkeys[$component['cid']] = $component['form_key'];
  }
  return $formkeys;
}

/**
 * Helper function that retrieves the values to be used with the user
 * registration. This is front-end and end-user.
 */
function _webform_registration_get_fields($formdata, $settings) {
  if (is_array($settings['account_fields'])) {
    $settings = array_merge($settings, $settings['account_fields']);
  }
  $fields = array();

  // Retrieve a list of the keys allowed to be fetched recursively
  $form_fields = _webform_registration_get_valid_form_fields();
  $special_cases = _webform_registration_get_special_cases();
  
  // Iterate over all the webform_registration fields and fetch field keys and values.
  foreach ($settings as $form_key => $cid) {
    // $form_key is allowed and $cid is not a special case.
    if (in_array($form_key, $form_fields) && !in_array($cid, $special_cases)) {
      $fields[$form_key] = _webform_registration_get_form_field($cid, $formdata);
    }
    elseif (in_array($cid, $special_cases)) {
      $fields[$form_key] = array('cid' => $cid, 'value' => $cid, 'field' => NULL);
      // @ToDo: Plug here the logic to handle special cases like <auto> for $form_key == 'pass';
    }
    else {
      $fields[$form_key] = $cid;
    }
  }
  return $fields;
}

/**
 * Takes a field reference and returns the value.
 */
function _webform_registration_get_form_field($cid, &$formdata, $value = NULL) {
  global $user;
  if (!isset($cid) || is_null($cid) || $cid == '<none>' || $cid == '<auto>') {
    // This is an exception for special cases like <auto> or <none>.
    // It also handles developer ooopsies like passing an empty $cid.
    return;
  }

  // Assign a value only if there is one to be assigned.
  if ($value == NULL && isset($formdata[$cid])) {
    $value = $formdata[$cid];
  }

  $ret = array(
    'cid' => $cid,
    'field' => NULL,
    'value' => ($value) ? $value : NULL,
  );

  return $ret;
}

/**
 * Iterates over the $fields and assigns their values to the $formdata.
 */
function _webform_registration_set_fields(&$formdata, &$fields, $formkeys) {
  // These fields come from the actual rendered webform
  $form_fields = _webform_registration_get_valid_form_fields();
  // Iterate over all the webform_registration fields and fetch field keys and values.
  foreach ($fields as $form_key => $cid) {
    if (in_array($form_key, $form_fields)) {
      if (!isset($item['field'])) {
        $item['field'] = '';
      }
      _webform_registration_get_form_field($item['cid'], $formdata, $item['field']);
    }
  }
}

/**
 * Retrieves an array of only the fields a user can modify.
 * Useful for fetching from the registration webform.
 */
function _webform_registration_get_valid_form_fields() {

  $user_field_info_instances = field_info_instances('user', 'user');
  $valid_form_fields = backdrop_map_assoc(array(
    'name',
    'mail',
    'mail_confirm',
    'pass',
    'pass_confirm',
  ));

  return array_merge($valid_form_fields, array_keys($user_field_info_instances));
}

/**
 * Returns an array of special cases used when we need to get data from places
 * other than the form ($formdata).
 */
function _webform_registration_get_special_cases() {
  return array(
    '<none>',
    '<auto>',
  );
}
