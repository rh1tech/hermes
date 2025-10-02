const fs = require("fs-extra");
const path = require("path");

const packHtml = () => {
    const bundle = fs.readFileSync(path.resolve(__dirname, "dist/bundle.js"), "utf8");
    const html = fs.readFileSync(path.resolve(__dirname, "src/template.html"), "utf8").replace(/\[\[bundle\]\]/ig, bundle);
    fs.writeFileSync(path.resolve(__dirname, "dist/index.html"), html);
    fs.unlinkSync(path.resolve(__dirname, "dist/bundle.js"));
    fs.renameSync(path.resolve(__dirname, "dist/index.html"), path.resolve(__dirname, "dist/index.html"));
};

packHtml();