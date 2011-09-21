<?php
/**
 * @file
 * Webform Registration Include File.
 *
 * This file holds all the end-user-facing functionality.
 * This file is only here to make the module easier to read.
 */

/**
 * Extends hook_form_alter() functionality.
 * Alters instances of registration enabled webforms.
 */
function webform_registration_inc_form_alter(&$form, &$form_state, $form_id) {
  global $user;

  $nid = $form['details']['nid']['#value'];
  $settings = _webform_registration_get_settings($nid);
  if (!$settings['enabled']) {
    return;
  }

  $haystack = & $form['submitted'];
  $fields = _webform_registration_get_fields($haystack, $settings);

  // Username field handling.
  if (empty($fields['name']['field'])) {
    drupal_set_message(t('Field %field not found', array('%field' => $settings['name'])), 'error');
  }

  // Password field handling.
  if ($settings['pass'] != '<auto>') {
    $fields['pass']['field']['#type'] = 'password';
  }

  // Password confirmation handling
  if ($settings['pass_confirm'] == '<none>') {
    // We just don't show a confirmation field.
  }
  elseif ($settings['pass_confirm'] == $settings['pass']) {
    // If they picked the same field for confirmation. We clone the pass field
    $fields['pass']['field']['#type'] = 'password';
    // Now we clone the pass field.
    $fields['pass_confirm']['field'] = $fields['pass']['field'];
    $fields['pass_confirm']['field']['#title'] = t('Password Confirmation');
    $fields['pass_confirm']['field']['#description'] = t('Please type your password again.');
    //$haystack[$settings['pass_confirm'] . '_confirm'] = $fields['pass_confirm']['field'];
  }
  elseif ($fields['pass_confirm']['field']['#type'] == 'textfield') {
    $fields['pass_confirm']['field']['#type'] = 'password';
    if (empty($fields['pass_confirm']['field']['#description'])) {
      $fields['pass_confirm']['field']['#description'] = t('Please type your password again.');
    }
  }

  // Email field handling
  if (isset($fields['mail']['field'])) {
    $fields['mail']['field']['#required'] = TRUE;
  }
  else {
    drupal_set_message(t('Field %field not found', array('%field' => $settings['mail'])), 'error');
  }

  // Email confirmation field handling
  if ($settings['mail_confirm'] == '<none>') {
    // We just don't show a confirmation field.
  }
  elseif ($settings['mail_confirm'] == $settings['mail']) {
    // If they picked the same field for confirmation. We clone the mail field
    $fields['mail_confirm']['field'] = $fields['mail']['field'];
    $fields['mail_confirm']['field']['#title'] = t('Email Confirmation');
    $fields['mail_confirm']['field']['#description'] = t('Please type your email again.');
  }
  else {
    // They specified a different field for mail confirmation.
    // @ToDo: Implement this usecase.
  }
  // @ToDo: Optionally. Implement javascript strength notification.
  // The user is logged in. QUICK! HIDE!
  if (user_is_logged_in()) {
    // Hide everything but populate the fields in case webform needs them for
    // something, e.g. Connecting with a CRM or such.

    $fields['name']['field']['#type'] = 'value';
    $fields['name']['field']['#value'] = $user->name;

    // We are not populating the password.
    $fields['pass']['field']['#type'] = 'value';
    $fields['pass']['field']['#value'] = '';

    $fields['pass_confirm']['field']['#type'] = 'value';
    $fields['pass_confirm']['field']['#value'] = '';
    $fields['pass_confirm']['field']['#required'] = FALSE;

    $fields['mail']['field']['#attributes'] = array('readonly' => 'readonly');
    $fields['mail']['field']['#value'] = $user->mail;

    $fields['mail_confirm']['field']['#type'] = 'value';
    $fields['mail_confirm']['field']['#value'] = $user->mail;
    $fields['mail_confirm']['field']['#required'] = FALSE;

    // @ToDo: Manage hiding fieldsets when all the fields are gone.
    // $form['#submit'][]    = 'webform_registration_submit_login_only';
    // $form['#validate'][]  = 'webform_registration_validate_login_only';
  }
  else {
    $form['#validate'][] = 'webform_registration_validate';
    $form['#submit'][] = 'webform_registration_submit';
  }
  _webform_registration_set_fields($haystack, $fields);

  drupal_add_js(array('webform_registration' => $settings), 'setting');
}

/**
 * Webform Registration Validate Function.
 */
function webform_registration_validate($form, &$form_state) {

  $nid = $form['details']['nid']['#value'];
  $settings = _webform_registration_get_settings($nid);

  if (isset($form_state['values']['submitted'])) {
    $haystack = &$form_state['values']['submitted'];
  }

  $fields = _webform_registration_get_fields($haystack, $settings);

  // If webform_registration is enabled for this form
  if ($fields['enabled']) {
    $user_exists = _webform_registration_user_exists($fields);
    // ... and user exists
    if ($user_exists['match'] === TRUE) {
      // Password
      if (empty($fields['pass']['value'])) {
        form_set_error($fields['pass']['key'], t('Your password field can\'t be empty when logging in'));
        return;
      }
      $login_validates = user_load(array(
        'name' => $user_exists['name'],
        'uid' => $user_exists['uid'],
        'pass' => $fields['pass']['value'],
        ));
      if (!$login_validates) {
        form_set_error($fields['pass']['key'], t('Your password and email don\'t match.'));
        return;
      }
    }
    else {
      // Username
      if ($settings['user'] != '<none>' && empty($fields['user']['value'])) {
        form_set_error($fields['user']['key'], t('Username field is required'));
        return;
      }
      // Password
      if (empty($fields['pass']['value'])) {
        form_set_error($fields['pass']['key'], t('Your password field can\t be empty when loggin in'));
        return;
      }
      // Password Confirmation
      if ($settings['pass_confirm'] != '<none>' && $fields['pass']['value'] != $fields['pass_confirm']['value']) {
        form_set_error($fields['pass_confirm']['key'], t('Your password and confirmation values do not match'));
        return;
      }
      // Email Confirmation
      if ($settings['mail_confirm'] != '<none>' && $fields['mail']['value'] != $fields['mail_confirm']['value']) {
        form_set_error($fields['mail_confirm']['key'], t('Your email and confirmation addresses do not match'));
        return;
      }

      if (_webform_registration_user_exists($fields, TRUE)) {
        form_set_error($fields['user']['key'], t('That username already exist'));
        return;
      }

      // Username Validation
      _webform_registration_map_registration_fields($form_state, $fields, $settings);
      user_register_validate($form, $form_state);
    }
  }
}

/**
 * Webform Registration Submit Handler.
 */
function webform_registration_submit($form, &$form_state) {
  $nid = $form['details']['nid']['#value'];
  $settings = _webform_registration_get_settings($nid);

  if (!$settings['enabled'] || user_is_logged_in()) {
    return;
  }

  if (isset($form_state['values']['submitted_tree'])) {
    $haystack = &$form_state['values']['submitted_tree'];
  }

  // Retrieve the values and/or form references
  $fields = _webform_registration_get_fields($haystack, $settings);

  // Do we need to register or login the user?
  $user_exists = _webform_registration_user_exists($fields);

  if ($user_exists['match'] === TRUE) {
    watchdog('webform_registration', 'Webform Registration: Login Triggered. %userid', array('%userid' => $user_exists['uid']));
    $form_state['values']['name'] = $fields['name']['value'] = $user_exists['name'];

    $user_form = _webform_registration_map_registration_fields($form_state, $fields);
    $user = user_authenticate($user_form['values']);
  }
  else {
    watchdog('webform_registration', 'Webform Registration: Registration Triggered');
    $redirect = array_shift($form_state['redirect']);

    $user_form = _webform_registration_map_registration_fields($form_state, $fields);
    $user_form['values']['notify'] = FALSE;

    user_register_submit($form, $user_form);

    $form_state['redirect'] = $redirect;
  }

  // Content Profile Tie-in
  if (module_exists('content_profile')) {
    webform_registration_profile($form_state);
  }
}

function webform_registration_profile(&$form_state) {
  if (user_is_anonymous()) {
    return FALSE;
  }

  global $user;
  $content_types = content_profile_get_types('names');

  foreach ($content_types as $type => $name) {
    $node = new stdClass();
    $node->type = $type;
    $node->uid = $user->uid;
    $node->title = $user->name;

    if (content_profile_profile_exists($node, $user->uid)) {
      $node = node_load((array) $node);
    }
    else {
      node_save($node);
    }

    foreach ($node as $key => $value) {
      if (strstr($key, 'field')) {
        $needle = str_replace('field_', '', $key);
        if ($found = _find_in_array($needle, $form_state['values']['submitted_tree'])) {
          $node->{$key}[0]['value'] = $found;
        }
      }
    }
    node_save($node);
  }
}

/**
 * Function that takes the values for the form, finds the email, and checks if
 * the user already exists.
 */
function _webform_registration_user_exists($fields, $check_both = FALSE) {
  $mail = webform_registration_email_exists($fields['mail']['value']);
  $ret = $mail['match'];
  if ($check_both == TRUE) {
    $uname = webform_registration_user_exists($fields['user']['value']);
    $ret = $uname['match'] || $ret;
  }
  return $ret;
}

/*
 * Recursively tries to find a value in nested arrays
 */

function _find_in_array($needle, &$array) {
  foreach ($array as $key => $value) {
    if (is_array($value)) {
      if ($found = _find_in_array($needle, $value)) {
        return $found;
      }
    }
    if ($key === $needle) {
      return $value;
    }
  }
  return FALSE;
}

/**
 * Checks if a given email exists in the name or mail columns of the users table.
 */
function webform_registration_email_exists($string) {
  $match = array('match' => FALSE);
  if (!empty($string)) {
    $result = db_query("SELECT uid, name
    FROM {users}
    WHERE LOWER(mail) = LOWER('%s') LIMIT 1;
    ", $string);
    while ($user = db_fetch_array($result)) {
      $match = array(
        'match' => TRUE,
        'uid' => $user['uid'],
        'name' => $user['name'],
      );
    }
  }
  return $match;
}

/**
 * Checks if a given username exists in the name or mail columns of the users table.
 */
function webform_registration_user_exists($string) {
  $match = array('match' => FALSE);
  if (!empty($string)) {
    $result = db_query("SELECT uid, name
    FROM {users}
    WHERE LOWER(name) = LOWER('%s') LIMIT 1;
    ", $string);
    while ($user = db_fetch_array($result)) {
      $match = array(
        'match' => TRUE,
        'uid' => $user['uid'],
        'name' => $user['name'],
      );
    }
  }
  return $match;
}

/**
 * Helper function that retrieves the values to be used with the user
 * registration. This is front-end and end-user.
 */
function _webform_registration_get_fields(&$haystack, $settings) {
  if (empty($settings['enabled'])) {
    return $settings;
  }

  $fields = array();

  // Retrieve a list of the keys allowed to be fetched recursively
  $form_fields = _webform_registration_get_valid_form_fields();
  $special_needles = _webform_registration_get_special_needles();

  // Iterate over all the webform_registration fields and fetch field keys and values.
  foreach ($settings as $key => $needle) {
    // $key is allowed and $needle is not a special needle.
    if (in_array($key, $form_fields) && !in_array($needle, $special_needles)) {
      // go fetch recursively the $haystack.
      $fields[$key] = _webform_registration_get_form_field($needle, $haystack);
    }
    elseif (in_array($needle, $special_needles)) {
      $fields[$key] = array('key' => $needle, 'value' => $needle, 'field' => NULL);
      // @ToDo: Plug here the logic to handle special needles like <auto> for $key == 'pass';
    }
    else {
      $fields[$key] = $needle;
    }
  }
  return $fields;
}

/**
 * Takes a field reference, that may or may not contain ][ separators to
 * determine their depth within the form and returns the value.
 */
function _webform_registration_get_form_field($needle, &$haystack, $value = NULL) {
  if (!isset($needle) || is_null($needle) || $needle == '<none>' || $needle == '<auto>') {
    // This is an exception for special needles like <auto> or <none>.
    // It also handles developer ooopsies like passing an empty $needle.
    return;
  }

  $path = explode('][', $needle);
  $depth = count($path);
  $field = &$haystack;
  $i = 0;
  foreach ($path as $item) {
    if (isset($field[$item])) {
      $field = &$field[$item];
      $i++;
    }
  }

  // Assign a value only if there is one to be assigned and we have a match in
  // the $haystack.
  if ($value != NULL && $depth == $i) {
    $field = $value;
  }

  $ret = array(
    'key' => $needle,
    'field' => (is_array($field) ? $field : NULL),
    'value' => (is_array($field) ? $field['#value'] : $field),
  );

  return $ret;
}

/**
 * Finds a form field with depth and sets the array that defines it.
 */
function _webform_registration_set_form_field($needle, &$haystack, $value) {
  _webform_registration_get_form_field($needle, $haystack, $value);
}

/**
 * Maps user entered form values into the $form_state array where user module expects them to be.
 */
function _webform_registration_map_registration_fields(&$form_state, $fields) {
  $user_form = array();

  // Prepare fields for validation and map them out into the values
  // These are the keys that the user module expects for validation and insertion.
  if (!empty($fields['mail']['value'])) {
    $form_state['values']['mail'] = $fields['mail']['value'];
  }
  if ($fields['name']['key'] != '<none>') {
    $form_state['values']['name'] = $fields['name']['value'];
  }
  else {
    $form_state['values']['name'] = $fields['mail']['value'];
  }

  // If pass equals <auto> then just assign '' so that the user module can generate that for us.
  $form_state['values']['pass'] = ($fields['pass']['key'] != '<auto>' ? $fields['pass']['value'] : '');

  // User registrations frowns when it sees a uid key.
  $registration = array_flip(array_keys(user_register()));
  $user_form['values'] = array_intersect_key($form_state['values'], $registration);

  return $user_form;
}

/**
 * Iterates over the $fields and assigns their values to the $haystack.
 */
function _webform_registration_set_fields(&$haystack, &$fields) {
  // These fields come from the actual rendered webform
  $form_fields = _webform_registration_get_valid_form_fields();
  // Iterate over all the webform_registration fields and fetch field keys and values.
  foreach ($fields as $key => $item) {
    if (in_array($key, $form_fields)) {
      _webform_registration_set_form_field($item['key'], $haystack, $item['field']);
    }
  }
}

/**
 * Returns a specific portion of a needle path;
 */
function _webform_registration_get_needle_part($needle, $part = 'all') {
  $path = explode('][', $needle);
  $depth = count($path);

  // Translates $part into actual numeric values.
  switch ($part) {
    case 'first':
      $part = 0;
      break;
    case 'last':
      $part = $depth - 1;
      break;
    case 'next to last':
      $part = $depth - 2;
      break;
    case 'all':
    default:
      // Not being a number will tell us to send the whole $path array.
      $part = 'all';
      break;
  }

  // $part must be numeric now.
  if ($part === 'all') {
    return $path;
  }
  else {
    return $path[$part];
  }
}

/**
 * Retrieves an array of only the fields a user can modify.
 * Useful for fetching from the registration webform.
 */
function _webform_registration_get_valid_form_fields() {
  return array(
    'name',
    'mail',
    'mail_confirm',
    'pass',
    'pass_confirm',
  );
}

/**
 * Returns an array of special needles used when we need to get data from places
 * other than the form ($haystack).
 */
function _webform_registration_get_special_needles() {
  return array(
    '<none>',
    '<auto>',
  );
}