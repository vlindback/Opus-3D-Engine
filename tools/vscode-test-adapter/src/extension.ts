import * as vscode from 'vscode';
import * as path from 'path';
import * as fs from 'fs';
import * as cp from 'child_process';

let diagnostics: vscode.DiagnosticCollection | undefined;
let discoveryTimeout: NodeJS.Timeout | undefined;

/* ---------------------------------------------------------
 * Entry point
 * --------------------------------------------------------- */
export function activate(context: vscode.ExtensionContext) {
	const controller = vscode.tests.createTestController(
		"opus3dTestController",
		"Opus3D Tests"
	);
	context.subscriptions.push(controller);

	diagnostics = vscode.languages.createDiagnosticCollection("opus3d-tests");
	context.subscriptions.push(diagnostics);

	controller.refreshHandler = async () => {
		await discoverTests(controller);
	};

	controller.createRunProfile(
		"Run Tests",
		vscode.TestRunProfileKind.Run,
		(request, token) => runTests(controller, request, token)
	);

	controller.createRunProfile(
		"Debug Test",
		vscode.TestRunProfileKind.Debug,
		(request, token) => debugTests(controller, request, token)
	);

	// Watch build directory for changes (rebuilds, new exe, etc.)
	const workspace = vscode.workspace.workspaceFolders?.[0];
	if (workspace) {
		const pattern = new vscode.RelativePattern(workspace, "build/**");
		const watcher = vscode.workspace.createFileSystemWatcher(pattern);
		watcher.onDidChange(() => scheduleTestDiscovery(controller));
		watcher.onDidCreate(() => scheduleTestDiscovery(controller));
		watcher.onDidDelete(() => scheduleTestDiscovery(controller));
		context.subscriptions.push(watcher);
	}

	// Initial discovery
	discoverTests(controller);
}

/* ---------------------------------------------------------
 * Meson introspection to find test executable
 * --------------------------------------------------------- */
function findTestExe(): string | undefined {
	const workspace = vscode.workspace.workspaceFolders?.[0];
	if (!workspace) return undefined;

	const workspaceRoot = workspace.uri.fsPath;
	const config = vscode.workspace.getConfiguration("opus3dTests");
	const userConfig = config.get<string>("buildConfig", "");
	const targetName = config.get<string>("targetName", "Opus3D-Test");

	const buildRoot = path.join(workspaceRoot, "build");
	if (!fs.existsSync(buildRoot)) return undefined;

	const candidateBuildDirs: string[] = [];

	if (userConfig) {
		candidateBuildDirs.push(path.join(buildRoot, userConfig));
	} else {
		for (const entry of fs.readdirSync(buildRoot, { withFileTypes: true })) {
			if (entry.isDirectory()) {
				candidateBuildDirs.push(path.join(buildRoot, entry.name));
			}
		}

		candidateBuildDirs.sort((a, b) => {
			const ad = /debug/i.test(path.basename(a));
			const bd = /debug/i.test(path.basename(b));
			return ad === bd ? 0 : (ad ? -1 : 1);
		});
	}

	for (const configDir of candidateBuildDirs) {
		const introPath = path.join(configDir, "meson-info", "intro-targets.json");
		if (!fs.existsSync(introPath)) continue;

		try {
			const targets = JSON.parse(fs.readFileSync(introPath, "utf8")) as any[];
			const target = targets.find(t => t.name === targetName && t.type === "executable");
			if (!target) continue;

			const filenames = Array.isArray(target.filename)
				? target.filename
				: [target.filename];

			if (!filenames.length) continue;

			const relOrAbs = filenames[0];
			let exePath: string;

			if (path.isAbsolute(relOrAbs)) {
				exePath = relOrAbs;
			} else {
				exePath = path.join(configDir, relOrAbs);
			}

			if (fs.existsSync(exePath)) {
				return exePath;
			}
		} catch {
			continue;
		}
	}

	return undefined;
}

/* ---------------------------------------------------------
 * Test discovery + auto-refresh
 * --------------------------------------------------------- */
function scheduleTestDiscovery(controller: vscode.TestController) {
	if (discoveryTimeout) {
		clearTimeout(discoveryTimeout);
	}
	// Debounce slightly so multiple file changes don't spam discovery
	discoveryTimeout = setTimeout(() => {
		discoverTests(controller);
	}, 500);
}

async function discoverTests(controller: vscode.TestController) {
	controller.items.replace([]);
	diagnostics?.clear();

	const exe = findTestExe();
	if (!exe) {
		vscode.window.showWarningMessage("Opus3D Tests: No test executable found.");
		return;
	}

	let output = "";
	try {
		output = cp.execSync(`"${exe}" --list`).toString();
	} catch {
		vscode.window.showErrorMessage("Opus3D Tests: Failed to query test list.");
		return;
	}

	parseTestList(output, controller);
}

function parseTestList(output: string, controller: vscode.TestController) {
	const lines = output.split(/\r?\n/);

	for (const line of lines) {
		const trimmed = line.trim();
		if (!trimmed.startsWith("TEST ")) continue;

		const fullName = trimmed.substring("TEST ".length).trim();
		// Example: "FoundationFibers.BasicSwitchReturn"

		const [suiteName, testName] = fullName.split(".");

		let suite = controller.items.get(suiteName);
		if (!suite) {
			suite = controller.createTestItem(suiteName, suiteName);
			controller.items.add(suite);
		}

		const test = controller.createTestItem(fullName, testName);
		suite.children.add(test);
	}
}

/* ---------------------------------------------------------
 * Running tests (with duration + failure parsing)
 * --------------------------------------------------------- */
async function runTests(
	controller: vscode.TestController,
	request: vscode.TestRunRequest,
	token: vscode.CancellationToken
) {
	const run = controller.createTestRun(request);
	diagnostics?.clear();

	// Start with the user's request (or the entire controller collection)
	const itemsOrArrays = request.include ?? controller.items;

	let flatTests: vscode.TestItem[] = [];

	// Iterate over the mixed collection (which yields TestItem OR [string, TestItem])
	for (const item of itemsOrArrays) {

		let testItem: vscode.TestItem;
		// **RUNTIME TYPE CHECK (Type Guard):**
		// Check if the element is an Array (i.e., a [string, TestItem] pair)
		if (Array.isArray(item)) {
			// It's the key-value pair, so we extract the value (the TestItem)
			testItem = item[1] as vscode.TestItem;
		} else {
			// It's a clean TestItem from request.include, so we use it directly
			testItem = item as vscode.TestItem;
		}

		// Check if the item has children (is a Suite)
		if (testItem.children.size > 0) {
			// If it's a suite, recursively flatten its children
			// The suite.children property is also an iterable yielding [string, TestItem]
			for (const [, childTest] of testItem.children) {
				flatTests.push(childTest);
			}
		} else {
			// If it has no children, it's a leaf test case
			flatTests.push(testItem);
		}
	}

	for (const test of flatTests) {
		if (token.isCancellationRequested) {
			break;
		}
		run.enqueued(test);
	}

	for (const test of flatTests) {
		if (token.isCancellationRequested) {
			break;
		}
		await runSingleTest(test, run, token);
	}

	run.end();
}

async function runSingleTest(
	test: vscode.TestItem,
	run: vscode.TestRun,
	token: vscode.CancellationToken
) {
	if (!test.parent) return;

	const exe = findTestExe();
	if (!exe) return;

	const fullName = test.id;

	run.started(test);
	const start = Date.now();

	let result: cp.SpawnSyncReturns<string>;
	try {
		result = cp.spawnSync(exe, ["--run", fullName], {
			encoding: "utf8",
			cwd: vscode.workspace.workspaceFolders?.[0]?.uri.fsPath
		});
	} catch (err) {
		const duration = Date.now() - start;
		const msg = new vscode.TestMessage("Failed to start test process.");
		run.failed(test, msg, duration);
		return;
	}

	const duration = Date.now() - start;
	const combinedOutput = (result.stdout || "") + (result.stderr || "");
	run.appendOutput(combinedOutput + "\n", test as any);

	const workspaceRoot = vscode.workspace.workspaceFolders?.[0]?.uri.fsPath;

	if (result.status === 0) {
		run.passed(test, duration);
	} else {
		const parsed = parseFailureOutput(combinedOutput, workspaceRoot);
		let primaryMessage: vscode.TestMessage;

		if (parsed.length > 0) {
			// Use the first parsed failure as the primary message
			const first = parsed[0];
			primaryMessage = new vscode.TestMessage(first.message);
			if (first.location) {
				primaryMessage.location = first.location;
			}
		} else {
			primaryMessage = new vscode.TestMessage("Test failed");
		}

		run.failed(test, primaryMessage, duration);

		// Apply diagnostics for rich editor decorations
		applyDiagnostics(parsed);
	}
}

/* ---------------------------------------------------------
 * Failure parsing & diagnostics
 * --------------------------------------------------------- */

/**
 * ParsedFailure represents one assertion/location from the output.
 */
interface ParsedFailure {
	file?: string;
	line?: number;
	column?: number;
	message: string;
	location?: vscode.Location;
}

/**
 * Try to parse failure lines from test output.
 *
 * Example supported format:
 *   path/to/file.cpp:123:45: Some assertion failed message
 */
function parseFailureOutput(output: string, workspaceRoot?: string): ParsedFailure[] {
	const failures: ParsedFailure[] = [];
	const lines = output.split(/\r?\n/);

	// Matches:
	// TEST_FAILED Suite.Test Message at ../../path/file.cpp:79
	const regex = /^TEST_FAILED\s+(?<test>[A-Za-z0-9_.]+)\s+(?<msg>.+?)\s+at\s+(?<file>.+):(?<line>\d+)$/;

	const exe = findTestExe();
	const exeDir = exe ? path.dirname(exe) : undefined;
	const buildConfigDir = exeDir ? path.dirname(exeDir) : undefined; // build/<config>

	for (const line of lines) {
		const m = regex.exec(line.trim());
		if (!m || !m.groups) continue;

		const file = m.groups["file"];
		const lineNum = parseInt(m.groups["line"], 10);
		const msg = m.groups["msg"];

		let absFile: string;

		if (path.isAbsolute(file)) {
			absFile = file;
		} else if (buildConfigDir) {
			// ✅ Resolve relative to build/<config> (e.g. build/windows-msvc-debug)
			absFile = path.resolve(buildConfigDir, file);
		} else if (workspaceRoot) {
			// Fallback if something weird happens
			absFile = path.resolve(workspaceRoot, file);
		} else {
			continue;
		}

		const uri = vscode.Uri.file(absFile);
		const pos = new vscode.Position(Math.max(0, lineNum - 1), 0);

		failures.push({
			file: absFile,
			line: lineNum,
			column: 1,
			message: msg,
			location: new vscode.Location(uri, pos)
		});
	}

	return failures;
}


/**
 * Apply diagnostics from parsed failures for rich editor decorations.
 */
function applyDiagnostics(parsed: ParsedFailure[]) {
	if (!diagnostics) return;

	const perFile = new Map<string, vscode.Diagnostic[]>();

	for (const failure of parsed) {
		if (!failure.file || failure.line === undefined || failure.column === undefined) {
			continue;
		}

		const uri = vscode.Uri.file(failure.file);
		const range = new vscode.Range(
			new vscode.Position(Math.max(0, failure.line - 1), Math.max(0, failure.column - 1)),
			new vscode.Position(Math.max(0, failure.line - 1), Math.max(0, failure.column - 1 + 1))
		);

		const diag = new vscode.Diagnostic(
			range,
			failure.message,
			vscode.DiagnosticSeverity.Error
		);

		const key = uri.toString();
		if (!perFile.has(key)) {
			perFile.set(key, []);
		}
		perFile.get(key)!.push(diag);
	}

	diagnostics.clear();

	for (const [uriStr, diags] of perFile.entries()) {
		diagnostics.set(vscode.Uri.parse(uriStr), diags);
	}
}

/* ---------------------------------------------------------
 * Debugging tests
 * --------------------------------------------------------- */
async function debugTests(
	controller: vscode.TestController,
	request: vscode.TestRunRequest,
	token: vscode.CancellationToken
) {
	if (!request.include || request.include.length !== 1) {
		vscode.window.showErrorMessage("Select exactly one test to debug.");
		return;
	}

	const test = request.include[0];
	if (!test.parent) return;

	const exe = findTestExe();
	if (!exe) return;

	const debugConfig: vscode.DebugConfiguration = {
		name: "Debug Opus3D Test",
		type: process.platform === "win32" ? "cppvsdbg" : "cppdbg",
		request: "launch",
		program: exe,
		args: ["--run", test.id],
		cwd: "${workspaceFolder}",
		stopAtEntry: false
	};

	await vscode.debug.startDebugging(undefined, debugConfig);
}
