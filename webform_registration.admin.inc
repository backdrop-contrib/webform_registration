<?php

/**
 * @file
 * Webform Registration Admin Include
 *
 * This file holds all the admin and non-user facing functionality.
 */

/**
 * Implementation of hook_admin().
 */
function webform_registration_admin() {
  $form = array();
  $form['placeholder'] = array(
    '#type' => 'item',
    '#value' => t('This is a placeholder for module wide settings to come'),
  );
  return system_settings_form($form);
}
