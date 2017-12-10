 /**
 * 
 */

navigationOpenFilters = {};
navigation_eval_js = null;
var gan_slider_datas = new Array();

Event.observe(window, 'load', function() {
		ganLoadForPlain();	
	}
);

function ganLoadForPlain() {	
	mainNav("gan_nav_left", {"show_delay":"100","hide_delay":"100"});
	mainNav("gan_nav_top", {"show_delay":"100","hide_delay":"100"});
	mainNav("gan_nav_right", {"show_delay":"100","hide_delay":"100"});
	ganInitSliders();	
	if (typeof(mec_navigation_urlhash) != 'undefined' && mec_navigation_urlhash){
		ganPrepareUrl();
	}
}

function ganPrepareUrl(){
	var hash_str = window.location.hash;
	if (hash_str){		
		var url = window.location.href;
		url = url.replace(hash_str, '');
				
		var hashes = hash_str.slice(1).split('&');
	    var vars = new Array();	    
	    var hash = new Object();
	    var gan_data = false;
	    var hash_str = '';
	    
	    for(var i = 0; i < hashes.length; i++)
	    {	        
	    	vars = hashes[i].split('=');
	        
	    	if (vars[0] == 'gan_data'){
	    		gan_data = true;
	    		continue;
	    	}
	    	
	        if (vars[0] != 'ajax' && vars[0] != 'gan_data'){
	        	hash[vars[0]] = vars[1];
	        }
	    }    
	    
	    for(var key in hash){
	    	if (hash.hasOwnProperty(key)){
	    		hash_str += key + '=' + hash[key] + '&';
	    	}
	    }
		
		if (typeof(setNavigationUrl) == 'function' && gan_data && hash_str){						
			hash_str += 'ajax=1';			
			setNavigationUrl(url + '?' + hash_str);
		}
	}
}

function ganInitSliders(){
	for(var i=0;i< gan_slider_datas.length;i++){
      $(gan_slider_datas[i].code+'-value-from').innerHTML = gan_slider_datas[i].from;
      $(gan_slider_datas[i].code+'-value-to').innerHTML = gan_slider_datas[i].to;
      $(gan_slider_datas[i].code+'-value').innerHTML = gan_slider_datas[i].htmlvalue;
    }
    gan_slider_datas = new Array();
} 


function showNavigationNote(id, control){
	
	var arr = $(control).cumulativeOffset();
	$(id).style.left = arr[0] + 'px'; 
	$(id).style.top = arr[1] + 'px';
	$(id).style.display = 'block';			
}

function hideNavigationNote(){
	
	$$('.filter-note-content').each(function(e){e.style.display = 'none';});
	
}


function navigationOpenFilter(request_var){
	
	var id = 'advancednavigation-filter-content-'+request_var;
	
	if( $(id).style.display == 'none' ){
		
		$(id).style.display = 'block';
		
		if (navigation_eval_js) {
			eval(navigation_eval_js);
			ganInitSliders();
		}	
		
		navigationOpenFilters[request_var+'_is_open'] = true;
		
	}else{
		
		$(id).style.display = 'none' ;
		
		navigationOpenFilters[request_var+'_is_open'] = false;
		
	}	
}