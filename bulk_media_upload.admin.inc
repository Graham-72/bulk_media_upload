<?php
/**
 * @file
 * Admin configuration page for bulk media upload
 *
 */

/**
 * Admin configuration page
 */
function bulk_media_upload_admin($form, &$form_state) {
  $form = array();

  $nodetypes = array();
  foreach (node_type_get_types() as $type) {
    $nodetypes[$type->type] = $type->name;
  }

  $form['bulk_media_upload_nodetype'] = array(
    '#type' => 'select',
    '#title' => t('Nodetype'),
    '#default_value' => variable_get('bulk_media_upload_nodetype'),
    '#description' => t('Which Nodetype should be generated'),
    '#required' => TRUE,
    '#options' => $nodetypes,
    '#ajax' => array(
      'callback' => 'bulk_media_upload_admin_ajax',
      'wrapper' => 'mediaelement-div',
      'method' => 'replace',
      'effect' => 'fade',
    ),
  );

  $form['mediafield-wrapper'] = array(
    '#prefix' => '<div id="mediaelement-div">',
    '#suffix' => '</div>',
  );

  if (isset($form_state['values']['bulk_media_upload_nodetype'])) {
    $nodetype = $form_state['values']['bulk_media_upload_nodetype'];
  }
  else {
    $nodetype = variable_get('bulk_media_upload_nodetype');
  }

  if ($nodetype) {
    $fields = field_info_instances('node', $nodetype);
    $mediafields = array();
    foreach ($fields as $field) {
      if ($field['widget']['type'] == 'media_generic') {
        $mediafields[$field['field_name']] = $field['label'] . ' (' . $field['field_name'] . ')';
      }
    }

    if (count($mediafields) > 0) {
      $form['mediafield-wrapper']['bulk_media_upload_mediafield'] = array(
        '#type' => 'select',
        '#title' => t('Mediafield'),
        '#default_value' => variable_get('bulk_media_upload_mediafield'),
        '#description' => t('Select the mediafield which should be filled with the uploaded files'),
        '#required' => TRUE,
        '#options' => $mediafields,
      );
    }
    else {
      $form['mediafield-wrapper']['bulk_media_upload_mediafield_info'] = array(
        '#markup' => '<p>There are no mediafields assigned to nodetype <strong>' . $nodetype . '</strong></p>',

      );
    }
  }

  return system_settings_form($form);
}

/**
 * Implements hook_validate().
 */
function bulk_media_upload_admin_validate($form, &$form_state) {
  if ($form_state['submitted']) {
    if (!isset($form_state['values']['bulk_media_upload_mediafield']) || $form_state['values']['bulk_media_upload_mediafield'] == '') {
      form_set_error('', t('You must select a mediafield'));
    }
  }
}

/**
 * Returns media field wrapper for ajax request
 */
function bulk_media_upload_admin_ajax($form, $form_state) {
  return $form['mediafield-wrapper'];
}
