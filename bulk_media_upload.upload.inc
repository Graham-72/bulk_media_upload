<?php
/**
 * @file
 * Upload form and node generation functions
 *
 */

/**
 * Form for bulk media upload
 */
function bulk_media_upload_upload_form($form, &$form_state) {
  global $language;

  $nodetype = variable_get('bulk_media_upload_nodetype');
  $mediafield = variable_get('bulk_media_upload_mediafield');

  if (empty($nodetype) || empty($mediafield)) {
    drupal_set_message(t('You have not configured the Bulk Media Upload module. Go to the ' . l(t('configuration page'), 'admin/config/media/bulk_media_upload') . ' and fix this!'), 'error');
    return $form;
  }

  $nodetype_info = node_type_load($nodetype);
  $mediafield_info = field_info_instance('node', $mediafield, $nodetype);

  $form['import_information'] = array(
    '#markup' => '<p>Nodetype used for generating the nodes: <strong>' . $nodetype_info->name . ' (' . $nodetype_info->type . ')</strong></p>'
    . '<p>Mediafield used for uploading your media: <strong>' . $mediafield_info['label'] . ' (' . $mediafield_info['field_name'] . ')</strong></p>',
  );

  $form['title'] = array(
    '#type' => 'textfield',
    '#title' => t('Title'),
    '#default_value' => '[file:name]',
    '#size' => 60,
    '#maxlength' => 255,
    '#required' => TRUE,
  );

  $form['token_help'] = array(
    '#title' => t('Replacement patterns'),
    '#type' => 'fieldset',
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
  );

  $form['token_help']['help'] = array(
    '#theme' => 'token_tree',
    '#token_types' => array('node', 'file'),
    '#global_types' => TRUE,
    '#click_insert' => TRUE,
  );

  $form['upload'] = array(
    '#type' => 'plupload',
    '#title' => t('Bulk Media Upload'),
    '#description' => t(''),
    '#required' => TRUE,
  );

  if (isset($mediafield_info['settings']['file_extensions'])) {
    $form['upload']['#upload_validators']['file_validate_extensions'][] = $mediafield_info['settings']['file_extensions'];
  }

  $form['default_values'] = array(
    '#type' => 'fieldset',
    '#title' => t('Default Values'),
    '#tree' => TRUE,
  );

  // Create the node creation form to get all assigned fields and add them to the default value fieldset.
  $allowed_fields = array_diff_key(field_info_instances('node', $nodetype), array($mediafield => NULL));

  $node = (object) array(
    'uid' => 1,
    'name' => '',
    'type' => $nodetype,
    'language' => LANGUAGE_NONE,
  );
  $nodeform = array();

  field_attach_form('node', $node, $nodeform, $form_state);

  // Delete Media field from default values.
  $form['default_values'] = array_merge($form['default_values'], array_intersect_key($nodeform, $allowed_fields));


  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Generate Nodes'),
  );
  return $form;
}

/**
 * Implements hook_validate().
 */
function bulk_media_upload_upload_form_validate($form, &$form_state) {
}

/**
 * Implements hook_submit().
 */
function bulk_media_upload_upload_form_submit($form, &$form_state) {

  // Prepare the batch process.
  $batch = array(
    'title' => t('Generating Nodes'),
    'error_message' => t('Node generation has encountered an error.'),
    'finished' => '_bulk_media_upload_batch_finished',
    'file' => drupal_get_path('module', 'bulk_media_upload') . '/bulk_media_upload.upload.inc',
  );

  foreach ($form_state['values']['upload'] as $tmpfile) {
    $batch['operations'][] = array('_bulk_media_upload_generate_node', array($tmpfile, $form, $form_state));
  }

  batch_set($batch);
}

/**
 * Internal function for node generation.
 */
function _bulk_media_upload_generate_node($tmpfile, $form, $form_state, &$context) {
  // Save media file.
  $scheme = variable_get('file_default_scheme', 'public') . '://';
  $source = $tmpfile['tmppath'];
  $destination = file_stream_wrapper_uri_normalize($scheme . $tmpfile['name']);
  $destination = file_unmanaged_move($source, $destination, FILE_EXISTS_RENAME);

  $file = file_uri_to_object($destination);
  file_save($file);
  _media_save_file_permenently($file);

  // Save node.
  $node = new stdClass();
  $node->type = variable_get('bulk_media_upload_nodetype');
  node_object_prepare($node);

  $node->title    = $file->filename;
  $node->language = LANGUAGE_NONE;

  node_save($node);

  // Media field.
  $mediafield_name = variable_get('bulk_media_upload_mediafield');
  $mediafield_value[LANGUAGE_NONE][0]['fid'] = $file->fid;
  $node->$mediafield_name = $mediafield_value;

  // Default values.
  field_attach_submit('node', $node, $form['default_values'], $form_state);

  node_save($node);

  // Replace tokens in title - this have to be done after node_save.
  $node->title = token_replace($form_state['values']['title'], array('node' => $node, 'file' => $file));

  node_save($node);

  $context['message'] = t('Importing: @filename', array('@filename' => $file->filename));
  $context['results']['nids'][] = $node->nid;
}

/**
 * Finished function for batch
 */
function _bulk_media_upload_batch_finished($success, $results, $operations) {
  drupal_set_message(t('Node generation completed'));
  $_SESSION['bulk_media_upload_batch_result'] = $results;
  drupal_goto('admin/content/media/bulk_upload/summary');
}
