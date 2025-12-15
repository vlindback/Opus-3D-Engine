import * as vscode from 'vscode';
import * as path from 'path';
import * as fs from 'fs';
import * as cp from 'child_process';

let diagnostics: vscode.DiagnosticCollection | undefined;
let discoveryTimeout: NodeJS.Timeout | undefined;
let lastResolvedExe: TestExecutableInfo | undefined;

interface TestExecutableInfo {
	exePath: string;
	buildConfigDir: string;
}

function computeBuildConfigDir(exePath: string): string {
	const exeDir = path.dirname(exePath);
	if (path.basename(exeDir).toLowerCase() === "tests") {
		return path.dirname(exeDir);
	}
	return exeDir;
}

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
function getWorkspaceRoot(): string | undefined {
	const workspace = vscode.workspace.workspaceFolders?.[0];
	return workspace?.uri.fsPath;
}

function resolveTestExe(): TestExecutableInfo | undefined {
	const workspaceRoot = getWorkspaceRoot();
	if (!workspaceRoot) return undefined;

	const config = vscode.workspace.getConfiguration("opus3dTests");
	const userConfig = config.get<string>("buildConfig", "");
	const targetName = config.get<string>("targetName", "Opus3D-Test");
	const explicitExe = config.get<string>("executablePath", "").trim();

	if (explicitExe) {
		const exePath = path.isAbsolute(explicitExe)
			? explicitExe
			: path.join(workspaceRoot, explicitExe);

		if (fs.existsSync(exePath)) {
			lastResolvedExe = {
				exePath,
				buildConfigDir: computeBuildConfigDir(exePath)
			};
			return lastResolvedExe;
		}
	}

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
				const resolved: TestExecutableInfo = {
					exePath,
					buildConfigDir: configDir
				};
				lastResolvedExe = resolved;
				return resolved;
			}
		} catch {
			continue;
		}
	}

	if (lastResolvedExe && fs.existsSync(lastResolvedExe.exePath)) {
		return lastResolvedExe;
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

	const exeInfo = resolveTestExe();
	if (!exeInfo) {
		vscode.window.showWarningMessage("Opus3D Tests: No test executable found.");
		return;
	}

	let output = "";
	try {
		output = cp.execSync(`"${exeInfo.exePath}" --list`).toString();
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

		const fullId = trimmed.substring("TEST ".length).trim();
		// Example: Foundation/Fibers/StressSwitching

		const parts = fullId.split("/");

		let parent: vscode.TestItem | undefined;
		let currentCollection = controller.items;

		let currentPath = "";

		for (let i = 0; i < parts.length; i++) {
			const part = parts[i];
			currentPath = currentPath ? `${currentPath}/${part}` : part;

			let item = currentCollection.get(currentPath);
			if (!item) {
				item = controller.createTestItem(
					currentPath,      // ID
					part              // Label
				);
				currentCollection.add(item);
			}

			parent = item;
			currentCollection = item.children;
		}
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
	diagnostics?.clear();
	const run = controller.createTestRun(request);

	const exeInfo = resolveTestExe();
	if (!exeInfo) {
		vscode.window.showWarningMessage("Opus3D Tests: No test executable found.");
		run.end();
		return;
	}

	/* -------------------------------------------------
	 * 1. Determine run prefix
	 * ------------------------------------------------- */
	let runPrefix = "";

	if (request.include && request.include.length === 1) {
		runPrefix = request.include[0].id;
	}

	const args = runPrefix ? ["--run", runPrefix] : [];

	/* -------------------------------------------------
	 * 2. Enqueue tests
	 * ------------------------------------------------- */
	function enqueue(item: vscode.TestItem) {
		if (item.children.size === 0) {
			run.enqueued(item);
		} else {
			for (const [, child] of item.children) {
				enqueue(child);
			}
		}
	}

	if (request.include) {
		for (const item of request.include) {
			enqueue(item);
		}
	} else {
		for (const [, item] of controller.items) {
			enqueue(item);
		}
	}

	/* -------------------------------------------------
	 * 3. Spawn test runner (single process)
	 * ------------------------------------------------- */
	const proc = cp.spawn(exeInfo.exePath, args, {
		cwd: getWorkspaceRoot(),
		stdio: ["ignore", "pipe", "pipe"]
	});

	token.onCancellationRequested(() => proc.kill());

	/* -------------------------------------------------
	 * 4. State tracking
	 * ------------------------------------------------- */
	const activeTests = new Map<string, vscode.TestItem>();

	let currentTest: vscode.TestItem | undefined;
	let failingTestId: string | null = null;
	let failureBuffer: string[] = [];

	/* -------------------------------------------------
	 * 5. Output handler
	 * ------------------------------------------------- */
	const handleLine = (line: string) => {
		const trimmed = line.trim();
		if (!trimmed) return;

		const isProtocol =
			trimmed.startsWith("TEST_START ") ||
			trimmed.startsWith("TEST_PASSED ") ||
			trimmed.startsWith("TEST_FAILED ") ||
			trimmed.startsWith("TEST_END ");

		// Only forward non-protocol output
		if (!isProtocol) {
			if (currentTest) {
				run.appendOutput(trimmed + "\n", undefined, currentTest);
			} else {
				run.appendOutput(trimmed + "\n");
			}
		}

		/* ---------- TEST_START ---------- */
		if (trimmed.startsWith("TEST_START ")) {
			const id = trimmed.substring("TEST_START ".length);
			const test = findTestById(controller, id);

			if (!test) return;

			currentTest = test;
			activeTests.set(id, test);
			run.started(test);
			return;
		}

		/* ---------- TEST_PASSED ---------- */
		if (trimmed.startsWith("TEST_PASSED ")) {
			const id = trimmed.substring("TEST_PASSED ".length);
			const test = activeTests.get(id);
			if (test) {
				run.passed(test);
				activeTests.delete(id);
			}
			currentTest = undefined;
			return;
		}

		/* ---------- TEST_SKIPPED ---------- */
		if (trimmed.startsWith("TEST_SKIPPED ")) {
			const rest = trimmed.substring("TEST_SKIPPED ".length);
			const firstSpace = rest.indexOf(" ");
			const id = firstSpace === -1 ? rest : rest.substring(0, firstSpace);
			const reason = firstSpace === -1 ? undefined : rest.substring(firstSpace + 1).trim();

			const test = activeTests.get(id);
			if (test) {
				run.skipped(test);
				if (reason) {
					run.appendOutput(`Skipped: ${reason}\n`, undefined, test);
				}
				activeTests.delete(id);
			}
			currentTest = undefined;
			failingTestId = null;
			failureBuffer = [];
			return;
		}

		/* ---------- TEST_FAILED ---------- */
		if (trimmed.startsWith("TEST_FAILED ")) {
			const rest = trimmed.substring("TEST_FAILED ".length);
			const firstSpace = rest.indexOf(" ");
			failingTestId = firstSpace !== -1 ? rest.substring(0, firstSpace) : rest;
			failureBuffer = [trimmed];
			return;
		}

		/* ---------- FAILURE DETAILS ---------- */
		if (failingTestId) {
			failureBuffer.push(trimmed);
		}

		/* ---------- TEST_END ---------- */
		if (trimmed.startsWith("TEST_END ")) {
			const id = trimmed.substring("TEST_END ".length);

			if (id === failingTestId) {
				const test = activeTests.get(id);
				if (test) {

					const parsed = parseFailureOutput(failureBuffer.join("\n"), exeInfo.buildConfigDir);

					const displayMessage = parsed.length
						? parsed[0].message
						: "Test failed";

					const msg = new vscode.TestMessage(
						new vscode.MarkdownString(
							[
								`**Assertion failed**`,
								"",
								displayMessage
							].join("\n")
						)
					);

					if (parsed.length && parsed[0].location) {
						msg.location = parsed[0].location;
						applyDiagnostics(parsed);
					}

					run.failed(test, msg);
					activeTests.delete(id);
					currentTest = undefined;
				}

				failingTestId = null;
				failureBuffer = [];
			}
			return;
		}
	};

	proc.stdout.on("data", d =>
		d.toString().split(/\r?\n/).forEach(handleLine)
	);
	proc.stderr.on("data", d =>
		d.toString().split(/\r?\n/).forEach(handleLine)
	);

	/* -------------------------------------------------
	 * 6. Process exit handling
	 * ------------------------------------------------- */
	await new Promise<void>(resolve => proc.on("close", () => resolve()));

	// Any test that started but never finished = crash
	for (const [, test] of activeTests) {
		run.errored(test, new vscode.TestMessage("Test runner exited unexpectedly"));
	}

	run.end();
}


function findTestById(
	controller: vscode.TestController,
	id: string
): vscode.TestItem | undefined {
	const parts = id.split("/");

	let collection = controller.items;
	let current: vscode.TestItem | undefined;
	let pathSoFar = "";

	for (const part of parts) {
		pathSoFar = pathSoFar ? `${pathSoFar}/${part}` : part;
		current = collection.get(pathSoFar);
		if (!current) return undefined;
		collection = current.children;
	}

	return current;
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
function parseFailureOutput(output: string, buildConfigDir?: string): ParsedFailure[] {
	const failures: ParsedFailure[] = [];
	const lines = output.split(/\r?\n/);

	let currentFailure: ParsedFailure | null = null;

	for (const line of lines) {
		const trimmed = line.trim();

		if (trimmed.startsWith("TEST_FAILED ")) {
			const rest = trimmed.substring("TEST_FAILED ".length);

			// Try to extract inline file:line
			const inlineLoc = /File:\s+(.+):(\d+)/.exec(rest);

			let message = rest;
			if (inlineLoc) {
				message = rest.substring(0, inlineLoc.index).trim();
			}

			if (!message) {
				message = "Test failed";
			}

			currentFailure = {
				message
			};

			// If inline location exists, resolve it immediately
			if (inlineLoc) {
				const file = inlineLoc[1];
				const lineNum = parseInt(inlineLoc[2], 10);

				const resolvedFile = resolveFailurePath(file, buildConfigDir);

				currentFailure.file = resolvedFile;
				currentFailure.line = lineNum;
				currentFailure.column = 1;
				currentFailure.location = new vscode.Location(
					vscode.Uri.file(resolvedFile),
					new vscode.Position(lineNum - 1, 0)
				);

				failures.push(currentFailure);
				currentFailure = null;
			}

			continue;
		}

		// Support both formats:
		// at file.cpp:123
		// File: file.cpp:123
		let locMatch = /^at\s+(.+):(\d+)$/.exec(trimmed);
		if (!locMatch) {
			locMatch = /File:\s+(.+):(\d+)/.exec(trimmed);
		}

		if (locMatch && currentFailure) {
			const file = locMatch[1];
			const lineNum = parseInt(locMatch[2], 10);

			const resolvedFile = resolveFailurePath(file, buildConfigDir);

			currentFailure.file = resolvedFile;
			currentFailure.line = lineNum;
			currentFailure.column = 1;
			currentFailure.location = new vscode.Location(
				vscode.Uri.file(resolvedFile),
				new vscode.Position(lineNum - 1, 0)
			);

			failures.push(currentFailure);
			currentFailure = null;
		}
	}

	return failures;
}

function resolveFailurePath(file: string, buildConfigDir?: string): string {
	if (path.isAbsolute(file)) {
		return file;
	}

	if (buildConfigDir) {
		return path.resolve(buildConfigDir, file);
	}

	const workspaceRoot = getWorkspaceRoot();
	if (workspaceRoot) {
		return path.resolve(workspaceRoot, file);
	}

	return path.resolve(file);
}


/**
 * Apply diagnostics from parsed failures for rich editor decorations.
 */
function applyDiagnostics(parsed: ParsedFailure[]) {
	if (!diagnostics) return;

	const byUri = new Map<string, vscode.Diagnostic[]>();

	for (const failure of parsed) {
		if (!failure.file || failure.line === undefined || failure.column === undefined) {
			continue;
		}

		const uriKey = vscode.Uri.file(failure.file).toString();

		const range = new vscode.Range(
			new vscode.Position(Math.max(0, failure.line - 1), Math.max(0, failure.column - 1)),
			new vscode.Position(Math.max(0, failure.line - 1), Math.max(0, failure.column - 1 + 1))
		);

		const diag = new vscode.Diagnostic(
			range,
			failure.message,
			vscode.DiagnosticSeverity.Error
		);

		// Merge diagnostics per file instead of replacing (keep prior failures)
		const existing = byUri.get(uriKey) ?? [];
		existing.push(diag);
		byUri.set(uriKey, existing);
	}

	for (const [uriKey, diags] of byUri) {
		const uri = vscode.Uri.parse(uriKey);
		const existing = diagnostics.get(uri) ?? [];
		diagnostics.set(uri, existing.concat(diags));
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

	const exeInfo = resolveTestExe();
	if (!exeInfo) {
		vscode.window.showWarningMessage("Opus3D Tests: No test executable found.");
		return;
	}

	const debugConfig: vscode.DebugConfiguration = {
		name: "Debug Opus3D Test",
		type: process.platform === "win32" ? "cppvsdbg" : "cppdbg",
		request: "launch",
		program: exeInfo.exePath,
		args: ["--run", test.id],
		cwd: "${workspaceFolder}",
		stopAtEntry: false
	};

	await vscode.debug.startDebugging(undefined, debugConfig);
}
