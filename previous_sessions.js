 // this is the page that will change the html and css of the previous_sessions html_page

 $(document).ready(function() {

  $("form_itself").submit(function (event) {
   var formData = {
    pv_session_nr: $("#previous_sessions_nr").val(),
    pv_date: $("#previous_sesions_date").val()


   };
   $.ajax({
    type: "POST",
    url: "previous_sessions.php",
    data: formData,
    datatype: "json",
    encode: true,
   }).done(function (data) {
    console.log(data);
    if(!data.success){
     if(data.errors.pv_session_nr){
      $("#nr-group").addClass("has-error");
      $("#nr-group").append( '<div class="help-block">' + data.errors.pv_session_nr + "</div>");


     }

    }
    else{




    }


   });

   event.preventDefault()
  });

 });
