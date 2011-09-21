<?php
/**
 * @file
 * Template file for displaying registration with reduced regions and no sidebars.
 */
?>
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Strict//EN"
  "http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd">
<html xmlns="http://www.w3.org/1999/xhtml" xml:lang="<?php print $language->language ?>" lang="<?php print $language->language ?>" dir="<?php print $language->dir ?>">
  <head>
    <title><?php print $head_title ?></title>
    <?php print $head ?>
    <?php print $styles ?>
    <?php print $scripts ?>
    <!--[if lt IE 7]>
    <?php print theme('get_ie_styles'); ?>
    <![endif]-->
  </head>
  <body>
    <!-- Special Page Template: page-registration.tpl.php -->
    <!-- Layout -->
    <div id="wrapper" class="sweepstakes">
      <div id="container" class="clear-block">
        <div id="page">

          <?php if ($header): ?>
            <div id="header-blocks" class="region region-header">
              <?php print $header; ?>
            </div> <!-- /#header-blocks -->
          <?php endif; ?>

          <?php
          // @ToDo: Track what is removing the $messages variable.
          print $temp_messages;
          ?>

          <?php if ($page == 0): ?>
            <h2><a href="<?php print $node_url ?>" title="<?php print $title ?>"><?php print $title ?></a></h2>
          <?php endif; ?>
          <div id="tabs">
            <?php if ($tabs): print '<ul class="tabs primary">' . $tabs . '</ul>';
            endif; ?>
          </div>
          <?php if ($tabs2): print '<ul class="tabs secondary">' . $tabs2 . '</ul>';
          endif; ?>

          <?php print $content_top; ?>
<?php print $content ?>
<?php print $content_bottom; ?>

          <div class="clear-block clear">
            <div class="meta">
              <?php if ($taxonomy): ?>
                <div class="terms"><?php print $terms ?></div>
            <?php endif; ?>
            </div>
            <?php if ($links): ?>
              <div class="links"><?php print $links; ?></div>
<?php endif; ?>
          </div>
          <div class="clear-block clear"></div>
        </div>
      </div> <!-- close container -->
    </div> <!-- close wrapper -->
<?php print $closure ?>
  </body>
</html>