const path = require("path");
const webpack = require("webpack");
const TerserPlugin = require("terser-webpack-plugin");
const ESLintPlugin = require("eslint-webpack-plugin");
const CssMinimizerPlugin = require("css-minimizer-webpack-plugin");
const babelConfig = require("./babel.config");

module.exports = async (env, argv) => [
    {
        context: path.resolve(`${__dirname}`),
        performance: {
            maxEntrypointSize: 10485760, // 10 MB
            maxAssetSize: 52428800, // 50 MB
        },
        name: "Userspace",
        target: ["web", "es5"],
        output: {
            path: path.resolve(__dirname, "dist"),
            filename: "bundle.js",
            library: "bundle",
            libraryTarget: "var",
            publicPath: "",
        },
        devtool: argv.mode === "production" ? false : "inline-source-map",
        module: {
            rules: [
                {
                    test: /\.tsx?$/,
                    use: [
                        {
                            loader: "ts-loader",
                            options: {
                                exclude: /node_modules/,
                            },
                        },
                    ],
                },
                {
                    test: /\.(woff(2)?|ttf|eot|otf|png|jpg|jpeg|svg|mp4|webm|mp3)(\?v=\d+\.\d+\.\d+)?$/,
                    type: "asset/inline",
                },
                {
                    test: /\.(css|scss|sass)$/,
                    use: [
                        {
                            loader: "style-loader"
                        },
                        {
                            loader: "css-loader",
                            options: {
                                importLoaders: 1,
                                sourceMap: false,
                                url: true,
                            },
                        },
                        {
                            loader: "postcss-loader",
                            options: {
                                postcssOptions: {
                                    config: path.resolve(
                                        __dirname,
                                        "postcss.config.js",
                                    ),
                                },
                            },
                        },
                        {
                            loader: "sass-loader",
                            options: {
                                sassOptions: {
                                    quietDeps: true,
                                },
                            },
                        },
                    ].filter((i) => i !== null),
                },
                {
                    test: /\.marko$/,
                    use: [
                        {
                            loader: "@marko/webpack/loader",
                            options: {
                                babelConfig: {
                                    ...babelConfig(),
                                },
                            },
                        },
                    ],
                },
                {
                    test: /\.js$/,
                    exclude: /node_modules/,
                    use: [
                        {
                            loader: "babel-loader",
                            options: {
                                cacheDirectory: true,
                                ...babelConfig(),
                            },
                        },
                    ],
                },
            ],
        },
        optimization: {
            usedExports: true,
            minimizer:
                argv.mode === "production"
                    ? [
                        new TerserPlugin({
                            parallel: true,
                            extractComments: false,
                            terserOptions: {
                                format: {
                                    comments: false,
                                },
                                compress: true,
                                sourceMap: false,
                                safari10: false,
                            },
                        }),
                        new CssMinimizerPlugin({
                            minimizerOptions: {
                                preset: [
                                    "default",
                                    {
                                        discardComments: {
                                            removeAll: true,
                                        },
                                    },
                                ],
                            },
                        }),
                    ]
                    : [],
        },
        plugins: [
            new webpack.DefinePlugin({
                "typeof window": "'object'",
                "process.browser": true,
            }),
            new ESLintPlugin({
                failOnError: true,
                failOnWarning: false,
            }),
        ],
        resolve: {
            alias: {
            },
            extensions: [".tsx", ".ts", ".js", ".marko"],
        },
        cache: {
            type: "filesystem",
            allowCollectingMemory: true,
            cacheDirectory: path.resolve(__dirname, ".cache"),
        },
    },
];
