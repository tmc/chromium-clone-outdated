// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/base_paths.h"
#include "base/files/file_util.h"
#include "base/path_service.h"
#include "chrome/browser/extensions/component_loader.h"
#include "chrome/browser/extensions/extension_apitest.h"
#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/extensions/manifest_handlers/mime_types_handler.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/browser/plugin_service.h"
#include "content/public/test/browser_test_utils.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/test/result_catcher.h"
#include "grit/browser_resources.h"
#include "net/test/embedded_test_server/embedded_test_server.h"

class PDFExtensionTest : public ExtensionApiTest {
 public:
  virtual ~PDFExtensionTest() {}

  virtual void SetUpCommandLine(base::CommandLine* command_line) override {
    ExtensionApiTest::SetUpCommandLine(command_line);
    command_line->AppendSwitch(switches::kEnableOutOfProcessPdf);
  }

  virtual void SetUpOnMainThread() override {
    ExtensionApiTest::SetUpOnMainThread();
    ASSERT_TRUE(embedded_test_server()->InitializeAndWaitUntilReady());
  }


  virtual void TearDownOnMainThread() override {
    ASSERT_TRUE(embedded_test_server()->ShutdownAndWaitUntilComplete());
    ExtensionApiTest::TearDownOnMainThread();
  }

  void RunTestsInFile(std::string filename) {
    base::FilePath pdf_path;
    ASSERT_TRUE(PathService::Get(chrome::FILE_PDF_PLUGIN, &pdf_path));
    ASSERT_TRUE(
        content::PluginService::GetInstance()->GetRegisteredPpapiPluginInfo(
            pdf_path));
    ExtensionService* service = extensions::ExtensionSystem::Get(
        profile())->extension_service();
    service->component_loader()->Add(IDR_PDF_MANIFEST,
        base::FilePath(FILE_PATH_LITERAL("pdf")));
    const extensions::Extension* extension =
        extensions::ExtensionRegistry::Get(profile())
            ->enabled_extensions()
            .GetByID("mhjfbmdgcfjbbpaeojofohoefgiehjai");
    ASSERT_TRUE(extension);
    ASSERT_TRUE(MimeTypesHandler::GetHandler(
        extension)->CanHandleMIMEType("application/pdf"));

    extensions::ResultCatcher catcher;

    GURL url(embedded_test_server()->GetURL("/pdf/test.pdf"));
    GURL extension_url(
        "chrome-extension://mhjfbmdgcfjbbpaeojofohoefgiehjai/index.html?" +
        url.spec());
    ui_test_utils::NavigateToURL(browser(), extension_url);
    content::WebContents* contents =
        browser()->tab_strip_model()->GetActiveWebContents();
    content::WaitForLoadStop(contents);

    base::FilePath test_data_dir;
    PathService::Get(base::DIR_SOURCE_ROOT, &test_data_dir);
    test_data_dir = test_data_dir.Append(
        FILE_PATH_LITERAL("chrome/test/data/pdf"));
    test_data_dir = test_data_dir.AppendASCII(filename);

    std::string test_js;
    ASSERT_TRUE(base::ReadFileToString(test_data_dir, &test_js));
    ASSERT_TRUE(content::ExecuteScript(contents, test_js));

    if (!catcher.GetNextResult())
      FAIL() << catcher.message();
  }
};

IN_PROC_BROWSER_TEST_F(PDFExtensionTest, Basic) {
  RunTestsInFile("basic_test.js");
}

IN_PROC_BROWSER_TEST_F(PDFExtensionTest, BasicPlugin) {
  RunTestsInFile("basic_plugin_test.js");
}

IN_PROC_BROWSER_TEST_F(PDFExtensionTest, Viewport) {
  RunTestsInFile("viewport_test.js");
}
