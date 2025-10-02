import index from "./entrypoint.marko";

const result = index.renderSync({});
result.appendTo(document.body);