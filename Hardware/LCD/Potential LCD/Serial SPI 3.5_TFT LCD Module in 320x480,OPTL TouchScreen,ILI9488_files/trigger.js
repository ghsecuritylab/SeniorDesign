/**
 * Trigger hover events for cart
 */
document.observe('dom:loaded', function() {
  $('cart-box-wrapper').observe('mouseover', function(e) {
    $('cart-box').show();
	$('cart-box').addClassName('cart-box-hover');
	$('cart-box-wrapper').addClassName('cart-box-wrapper-hover');

  });
	$('cart-box-wrapper').observe('mouseout', function(e) {
    $('cart-box').hide('slow');
	$('cart-box').removeClassName('cart-box-hover');
	$('cart-box-wrapper').removeClassName('cart-box-wrapper-hover');
  });
  
});

/**
 * Trigger hover events for wishlist
 */
document.observe('dom:loaded', function() {
  $('wishlist-wrapper').observe('mouseover', function(e) {
    $('wishlist-content').show();
	$('wishlist-content').addClassName('wishlist-content-hover');
	$('wishlist-wrapper').addClassName('wishlist-wrapper-hover');

  });
	$('wishlist-wrapper').observe('mouseout', function(e) {
    $('wishlist-content').hide('slow');
	$('wishlist-content').removeClassName('wishlist-content-hover');
	$('wishlist-wrapper').removeClassName('wishlist-wrapper-hover');
  });
  
});

