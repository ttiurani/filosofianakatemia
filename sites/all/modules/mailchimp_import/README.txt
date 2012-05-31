This module tries hard to take out the parts of your MailChimp campaigns you don't want to show on your website.  
In four ways:

1) mailchimp_import_strip_html() strips out all SCRIPT and STYLE tags and everything between them.  This gets
rid of the CSS embedded in your emails, which otherwise tend to mess up the page (eg, sometimes they contain
styles like "div#header { font-size: 90px };" and often you have a div#header on your website where you don't
happen to want 90px-tall lettering).
2) You get to specify the input format of imported campaigns on the MailChimp Import settings page 
at /admin/settings/mailchimp/import.  On import, mailchimp_import_strip_html() checks the input format's allowed
tags, and disallowed tags are stripped out of the campaigns with strip_tags().  This is helpful for getting rid 
of the !DOCTYPE, HTML and BODY tags, while not stripping out the content between them.
3) The module includes CSS to hide things like the campaign's header and footer, but you have to edit your 
MailChimp template and put those tags in.

	To do that, log into MailChimp and go to "My Templates".  Mouse over the template you want to use, and the options 
	"edit" and "code" appear.  Click on "code".  On the Custom Template Builder page this takes you to, there are three
	view options at top left: "code", "split", and "design".  Click on "code".  That lets you directly edit the HTML.
	Find the tags for any areas you want to hide, such as the <tr> tags for the header and footer, and add the CSS 
	class .mailchimp-hide, thus:
	
	  ...
		<table id="wrap" width="100%" border="0" cellspacing="0" cellpadding="0">
      <tr class="mailchimp-hide"> <!-- HEADER -->
      ...
 
 4) The CSS also includes "display:none"s to help hide embedded Facebook bits.
 
TODO: Make these features optional with checkboxes on the settings page.