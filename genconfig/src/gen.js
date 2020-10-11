const vm = require('vm');
const { readFileSync, promises: fs } = require('fs');
const Ajv = require('ajv');
const { read: readYaml } = require('yaml-import');

function objToPath(obj) {
    const epath = obj.errorPath;
    const path = epath.substring(2, epath.length - 1);

    return path.replace(/\//g, '.');
}

/**
 * Get value from a deep object by path.
 */
function getValue(obj, path) {
    path = path.split('.');
    for (let i = 0; i < path.length; i++) {
        obj = obj[path[i]];
        if (!obj) break;
    };
    return obj;
};

/**
 * Set value to an object by path.
 */
function setValue(obj, path, value) {
    path = path.split('.');
    let i;
    for (i = 0; i < path.length - 1; i++) {
        const tmp = obj[path[i]];
        if (!tmp) {
            tmp = {};
            obj[path[i]] = tmp;
        };
        obj = tmp;
    };

    obj[path[i]] = value;
}

exports.readSchemaFile = function readSchemaFile(path) {
    const schema = readYaml(path);

    return schema;
}

exports.readConfigFile = function readConfigFile(path) {
    const config = readYaml(path);

    return config;
}

exports.parseConfig = function parseConfig(schema, configObj) {
    const config = JSON.parse(JSON.stringify(configObj));
    const configKnobMap = {};
    const configMap = {};
    const dependencies = [];
    const ajv = new Ajv({
        removeAdditional: 'all',
        useDefaults: true,
        verbose: true,
        allErrors: true,
        extendRefs: true,
        strictDefaults: true,
        strictKeywords: true,
        strictNumbers: true,
        inlineRefs: false,
    });
    require('ajv-errors')(ajv);

    ajv.addKeyword('types', {
        type: 'object'
    });
    ajv.addKeyword('metaType', {
        macro: (type, parentSchema, obj) => {
            if (type === 'menu') {
                return {
                    type: 'object'
                };
            } else if (type === 'menuconfig') {
                return {
                    type: 'object',
                    required: ['enabled']
                };
            }

            const choice = parentSchema.choice;
            if (type === 'boolChoice' && choice) {
                for (const k in choice) {
                    configKnobMap[choice[k]] = {
                        path: objToPath(obj),
                        type: parentSchema.metaType,
                        choice: k,
                        makeOnly: parentSchema.makeOnly
                    };
                }

                return {
                    type: 'string',
                    enum: Object.keys(choice)
                }
            }

            if (!parentSchema.config) {
                return {
                    allOf: [
                        {},
                        { not: {} }
                    ]
                }
            }

            if (['int', 'hex', 'oct'].includes(type)) {
                return {
                    type: 'integer',
                };
            } else if (type === 'str') {
                return {
                    type: 'string',
                }
            } else if (type === 'bool') {
                return {
                    type: 'boolean',
                };
            } else if (type === 'tristate') {
                return {
                    type: 'string',
                    pattern: '^[nmy]$',
                };
            } else {
                return {
                    allOf: [
                        {},
                        { not: {} }
                    ],
                    errorMessage: `Invalid property type "${type}"`
                }
            }
        },
        metaSchema: {
            type: 'string'
        }
    });
    ajv.addKeyword('choice', {
        type: 'string',
        validate: (choice, value, parentSchema) => {
            if (!parentSchema.metaType) {
                return false;
            };

            configMap[choice[value]] = true;

            return true;
        },
        metaSchema: {
            type: 'object',
            patternProperties: {
                '^.*$': {
                    type: 'string'
                }
            },
            additionalProperties: false
        }
    });
    ajv.addKeyword('makeOnly', {
        dependencies: ['config'],
        metaSchema: {
            type: 'boolean'
        }
    });
    ajv.addKeyword('config', {
        dependencies: ['default', 'metaType'],
        compile: (name, parentSchema, obj) => {
            configKnobMap[name] = {
                path: objToPath(obj),
                type: parentSchema.metaType,
                makeOnly: parentSchema.makeOnly
            };

            // TODO Validate that config is under correct type of object
            return function validateConfig(value, dataPath) {
                const errors = [];
                if (configMap[name]) {
                    errors.push({
                        keyword: 'config',
                        message: `The config name exists: "${name}"`,
                        params: { keyword: 'config' }
                    });
                }

                if (!parentSchema.metaType) {
                    errors.push({
                        keyword: 'config',
                        message: 'metaType is not set',
                        params: { keyword: 'config' }
                    });
                };

                if (errors.length > 0) {
                    validateConfig.errors = errors;
                    return false;
                }

                configMap[name] = true;
                return true;
            };
        },
        metaSchema: {
            type: 'string'
        }
    });
    ajv.addKeyword('depends', {
        //dependencies: ['config'],
        validate: (...args) => {
            dependencies.push([args[2].config, args[0]]);
            return true;
        },
        metaSchema: {
            type: 'string'
        }
    });
    ajv.addKeyword('select', {
        modifying: true,
        errors: true,
        validate: function validateSelect(...args) {
            let sel = args[0];
            const val = args[1];
            const data = args[6];

            if (!val) return true;

            if (typeof sel === 'string') {
                sel = [{ knob: sel, expression: 'true' }];
            } else if (Array.isArray(sel) && typeof sel[0] === 'string') {
                sel = sel.map((s) => ({ knob: s, expression: 'true' }));
            } else if (!Array.isArray(sel)) {
                sel = [sel];
            }

            for (const { knob, value, expression } of sel) {
                if (evalExpression(data, expression)) {
                    if (!configKnobMap[knob]) {
                        validateSelect.errors = [
                            {
                                keyword: 'select',
                                message: `Select validation failed for ${knob}`,
                                params: { keyword: 'select' }
                            }
                        ];
                        return false;
                    }

                    const { path } = configKnobMap[knob];
                    setValue(data, path, value || true);
                }
            }

            return true;
        },
        metaSchema: {
            oneOf: [
                { type: 'string' },
                {
                    type: 'array',
                    uniqueItems: true,
                    items: { type: 'string' }
                },
                {
                    type: 'object',
                    properties: {
                        knob: { type: 'string' },
                        value: { not: { type: 'array' } },
                        expression: { type: 'string' },
                    },
                    required: ['knob'],
                    additionalProperties: false
                },
                {
                    type: 'array',
                    uniqueItems: true,
                    items: {
                        type: 'object',
                        properties: {
                            knob: { type: 'string' },
                            value: { not: { type: 'array' } },
                            expression: { type: 'string' }
                        },
                        required: ['knob'],
                        additionalProperties: false
                    }
                }
            ],
            additionalProperties: false
        }
    });

    function prepareSandbox(data) {
        const sandbox = {};
        for (const name in configKnobMap) {
            const { path, type, choice } = configKnobMap[name];

            const value = getValue(data, path);
            if (type === 'tristate') {
                sandbox[name] = value === 'n' ? false : value;
            } else if (type === 'boolChoice') {
                sandbox[name] = value === choice;
            } else {
                sandbox[name] = value;
            }
        }

        return sandbox;
    }

    function evalExpression(data, expression) {
        return !!vm.runInNewContext(expression, prepareSandbox(data));
    }

    function knob2Makefile(name) {
        const { path, type, choice } = configKnobMap[name];
        const value = getValue(config, path);

        if (value === undefined) {
            throw new Error(`${path} (${name}) is undefined`);
        }

        if ('str' === type) {
            if (value === '') {
                return `${name}=#`;
            }
            return `${name} = ${value}`;
        } else if ('bool' === type) {
            return `${name} = ${value ? 'y' : 'n'}`;
        } else if ('boolChoice' === type) {
            const v = value === choice;
            return `${name} = ${v ? 'y' : 'n'}`;
        }
        return `${name}=${value}`;
    }

    function knob2C(name) {
        const { path, type, choice, makeOnly } = configKnobMap[name];
        const value = getValue(config, path);
        if (makeOnly) return null;

        if ('str' === type) {
            return `#define ${name} "${value}"`
        } else if ('bool' === type) {
            if (!value) return null;
            return `#define ${name} 1`;
        } else if ('boolChoice' === type) {
            const v = value === choice;
            if (!v) return null;
            return `#define ${name} 1`;
        } else if (type === 'tristate') {
            let tri;

            switch (value) {
                case 'n': return null;
                case 'y': tri = 1; break;
                case 'm': tri = 2; break;
            }
            return `#define ${name} ${tri}`;
        }
        return `#define ${name} ${value}`;
    }

    const validate = ajv.compile(schema);
    const valid = validate(config);

    if (!valid) {
        const msg = JSON.stringify(validate.errors, null, 2);
        const err = new Error(msg);
        err.errors = JSON.parse(JSON.stringify(validate.errors));
        throw err;
    }

    const depErrors = [];
    let passed = true;
    for (const dependency of dependencies) {
        const [name, expression] = dependency;
        if (!evalExpression(config, `!${name} || (${expression})`)) {
            depErrors.push(`"${name}" depends on: "${expression}"`);
            passed = false;
        }
    }
    if (!passed) {
        const err = new Error(`Some dependencies failed: ${depErrors.join(', ')}`);
        err.dependencies = depErrors;
        throw err;
    }

    const mkLines = [];
    for (const k in configMap) {
        mkLines.push(knob2Makefile(k));
    }

    const hdrLines = [];
    for (const k in configMap) {
        const line = knob2C(k);
        if (line) hdrLines.push(line);
    }

    return {
        knobs: Object.keys(configKnobMap),
        config: config,
        makefile: mkLines.join('\n'),
        header: hdrLines.join('\n'),
    };
}