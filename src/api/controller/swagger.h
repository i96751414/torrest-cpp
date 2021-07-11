#ifndef TORREST_SWAGGER_H
#define TORREST_SWAGGER_H

#include "oatpp/web/server/api/ApiController.hpp"
#include "oatpp/core/macro/codegen.hpp"
#include "oatpp/core/macro/component.hpp"
#include "oatpp/parser/json/mapping/ObjectMapper.hpp"
#include "oatpp-swagger/Generator.hpp"

#define SWAGGER_DOC_URL "/api-docs/oas-3.0.0.json"
#define SWAGGER_RESOURCE(RES) "https://cdn.jsdelivr.net/npm/swagger-ui-dist@3.46.0/" RES

namespace torrest { namespace api {

#include OATPP_CODEGEN_BEGIN(ApiController)

class SwaggerController : public oatpp::web::server::api::ApiController {
public:
    SwaggerController(const std::shared_ptr<ObjectMapper> &pObjectMapper,
                      oatpp::Object<oatpp::swagger::oas3::Document> pDocument)
            : oatpp::web::server::api::ApiController(pObjectMapper),
              mDocument(std::move(pDocument)) {}

    static std::shared_ptr<SwaggerController> createShared(
            const std::shared_ptr<Endpoints> &pEndpointsList,
            OATPP_COMPONENT(std::shared_ptr<oatpp::swagger::DocumentInfo>, pDocumentInfo)) {

        auto serializerConfig = oatpp::parser::json::mapping::Serializer::Config::createShared();
        serializerConfig->includeNullFields = false;

        auto deserializerConfig = oatpp::parser::json::mapping::Deserializer::Config::createShared();
        deserializerConfig->allowUnknownFields = false;

        auto objectMapper = oatpp::parser::json::mapping::ObjectMapper::createShared(
                serializerConfig, deserializerConfig);

        std::shared_ptr<oatpp::swagger::Generator::Config> generatorConfig;
        try {
            generatorConfig = OATPP_GET_COMPONENT(std::shared_ptr<oatpp::swagger::Generator::Config>);
        } catch (const std::runtime_error &e) {
            generatorConfig = std::make_shared<oatpp::swagger::Generator::Config>();
        }

        oatpp::swagger::Generator generator(generatorConfig);
        auto document = generator.generateDocument(pDocumentInfo, pEndpointsList);
        return std::make_shared<SwaggerController>(objectMapper, document);
    }

    ENDPOINT("GET", SWAGGER_DOC_URL, api) {
        return createDtoResponse(Status::CODE_200, mDocument);
    }

    ENDPOINT("GET", "/swagger/ui", getUIRoot) {
        auto response = createResponse(Status::CODE_200, R"(<!DOCTYPE html>
<html lang="en">
  <head>
    <meta charset="UTF-8">
    <title>Swagger UI</title>
    <link rel="stylesheet" type="text/css" href=")" SWAGGER_RESOURCE("swagger-ui.min.css") R"(">
    <link rel="icon" type="image/png" href=")" SWAGGER_RESOURCE("favicon-32x32.png") R"(" sizes="32x32"/>
    <link rel="icon" type="image/png" href=")" SWAGGER_RESOURCE("favicon-16x16.png") R"(" sizes="16x16"/>
    <style>
      html {
        box-sizing: border-box;
        overflow: -moz-scrollbars-vertical;
        overflow-y: scroll;
      }

      *,
      *:before,
      *:after {
        box-sizing: inherit;
      }

      body {
        margin:0;
        background: #fafafa;
      }
    </style>
  </head>
  <body>
    <div id="swagger-ui"></div>
    <script src=")" SWAGGER_RESOURCE("swagger-ui-bundle.js") R"(" charset="UTF-8"></script>
    <script src=")" SWAGGER_RESOURCE("swagger-ui-standalone-preset.js") R"(" charset="UTF-8"></script>
    <script>
      window.onload = function() {
        const ui = SwaggerUIBundle({
          url: ")" SWAGGER_DOC_URL R"(",
          dom_id: '#swagger-ui',
          deepLinking: true,
          presets: [SwaggerUIBundle.presets.apis, SwaggerUIStandalonePreset],
          plugins: [SwaggerUIBundle.plugins.DownloadUrl],
          layout: "StandaloneLayout"
        })
        window.ui = ui
      }
    </script>
  </body>
</html>)");
        response->putHeader(Header::CONTENT_TYPE, "text/html");
        return response;
    }

private:
    oatpp::Object<oatpp::swagger::oas3::Document> mDocument;
};

#include OATPP_CODEGEN_END(ApiController)

}}

#endif //TORREST_SWAGGER_H
