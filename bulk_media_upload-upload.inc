<?php
function bulk_media_upload_upload_form($form, &$form_state) {
  global $language;
  
  $nodetype = variable_get('bulk_media_upload_nodetype','');
  $mediafield = variable_get('bulk_media_upload_mediafield','');
  
  if (empty($nodetype) || empty($mediafield)) {
    drupal_set_message(t('You have not configured the Bulk Media Upload module. Go to the ' . l('configuration page', 'admin/config/media/bulk_media_upload') . ' and fix this! As a result, this form is disabled.'), 'error');
    $disabled = TRUE;
  }else{
    $disabled = FALSE;
    $form['import_information'] = array(
      '#markup' => '<p>Nodetype used for generating the nodes: <strong>' . $nodetype . '</strong></p>'
                  .'<p>Mediafield used for uploading your media: <strong>' . $mediafield . '</strong></p>',
    );
  }
  
  $mediafield_info = field_info_instance('node', $mediafield, $nodetype);
  
  $form['upload'] = array(
    '#type' => 'plupload',
    '#title' => t('Bulk Media Upload'),
    '#description' => t(''),
    '#required' => TRUE,
    '#disabled' => $disabled
  );
  
  if(isset($mediafield_info['settings']['file_extensions'])){
    $form['upload']['#upload_validators']['file_validate_extensions'][] = $mediafield_info['settings']['file_extensions'];
  }
  if(!$disabled){
    $form['default_values'] = array(
      '#type' => 'fieldset',
      '#title' => t('Default Values'),
      '#tree' => TRUE
    );
    
    //Create the node creation form to get all assigned fields and add them to the defaul value fieldset
    
    $allowed_fields = array_diff_key(field_info_instances('node', $nodetype), array($mediafield => NULL));
    
    $node = (object) array(
      'uid' => 1,
      'name' => '',
      'type' => $nodetype,
      'language' => LANGUAGE_NONE,
    );
    $nodeform = array();
    
    field_attach_form('node', $node, $nodeform, $form_state);
    
    //Delete Media field from default values
    $form['default_values'] = array_merge($form['default_values'], array_intersect_key($nodeform, $allowed_fields));
  }
  
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Generate Nodes'),
    '#disabled' => $disabled,
  );
  return $form;
}


function bulk_media_upload_upload_form_validate($form, &$form_state) {
}

function bulk_media_upload_upload_form_submit($form, &$form_state){
  
  //Save the files
  $scheme = variable_get('file_default_scheme', 'public') . '://';
  $saved_files = array();
  // We can't use file_save_upload() because of http://www.jacobsingh.name/content/tight-coupling-no-not
  foreach ($form_state['values']['upload'] as $uploaded_file) {
    if ($uploaded_file['status'] == 'done') {
      $source = $uploaded_file['tmppath'];
      $destination = file_stream_wrapper_uri_normalize($scheme . $uploaded_file['name']);
      // Rename it to its original name, and put it in its final home.
      // Note - not using file_move here because if we call file_get_mime
      // (in file_uri_to_object) while it has a .tmp extension, it horks.

      $destination = file_unmanaged_move($source, $destination, FILE_EXISTS_RENAME);

      $file = file_uri_to_object($destination);
      file_save($file);
      _media_save_file_permenently($file);

      $saved_files[] = $file;
    }
    else {
      // @todo: move this to element validate or something and clean up t().
      form_set_error('pud', "Upload of {$uploaded_file['name']} failed");
    }
  }
  
  //Prepare the batch process
  
  $batch = array(
    'title' => t('Generating Nodes'),
    'error_message' => t('Node generation has encountered an error.'),
    'finished' => 'bulk_media_upload_batch_finished',
    //'file' => drupal_get_path('module', 'batch_example') . '/batch_example.inc',
    'file' => drupal_get_path('module', 'bulk_media_upload') . '/bulk_media_upload-upload.inc',
  );
  
  foreach ($saved_files as $file) {
    $batch['operations'][] = array('bulk_media_upload_generate_node', array($file->fid, $form_state['values']['default_values']));
  }
  
  batch_set($batch);
}

function bulk_media_upload_generate_node($fid, $default_values, &$context) {
  $file = file_load($fid);
  
  $node = new stdClass();
  $node->type = variable_get('bulk_media_upload_nodetype','');
  node_object_prepare($node);
  
  $node->title    = $file->filename;
  $node->language = LANGUAGE_NONE;

  $media = array('fid' => $fid, 'title' => NULL, 'data' => NULL);
  $node->field_media['und'][0] = $media;

  foreach($default_values as $field => $value){
    $node->$field = $value;
  }
  
  node_save($node);
  $context['message'] = t('Importing: @filename', array('@filename' => $fid));
  $context['results']['nids'][] = $node->nid;
}

function bulk_media_upload_batch_finished($success, $results, $operations) {
  drupal_set_message(t('Node generation completed'));
  $_SESSION['bulk_media_upload_batch_result'] = $results;
  drupal_goto('admin/content/media/bulk_upload/summary');
}

