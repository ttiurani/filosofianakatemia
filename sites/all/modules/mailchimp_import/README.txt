This module tries hard to take out the parts of your MailChimp campaigns you don't want to show on your website.  

1) mailchimp_import_strip_html() strips out all SCRIPT and STYLE tags and everything between them.  This gets
rid of the CSS embedded in your emails, which otherwise tend to mess up the page (eg, sometimes they contain
styles like "div#header { font-size: 90px };" and often you have a div#header on your website where you don't
happen to want 90px-tall lettering).
2) You get to specify the input format of imported campaigns on the MailChimp Import settings page 
at /admin/settings/mailchimp/import.  On import, mailchimp_import_strip_html() checks the input format's allowed
tags, and disallowed tags are stripped out of the campaigns with strip_tags().  This is helpful for getting rid 
of the !DOCTYPE, HTML and BODY tags, while not stripping out the content between them.
