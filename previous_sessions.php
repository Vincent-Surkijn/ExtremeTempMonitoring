<?php
$errors=[];
$data=[];


if(empty($_post['pv_date']) AND empty($_post['pv_sesssion_nr'])){
    $errors['pv_session_nr']="session_date or session_nr is required";



}
if(!empty($errors)){
    $data['success']=false;
    $data['errors']=$errors;



} else {
    $data['success']=true;
    $data['message']='succes';


}

echo json_encode($data);
