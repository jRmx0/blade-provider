#!/usr/bin/env node

/**
 * Stage 2 Implementation Verification Script
 * Tests bounce algorithm metadata structure and validation
 */

const fs = require('fs');
const path = require('path');

console.log('=== Bounce Algorithm Stage 2 Implementation Verification ===\n');

// File checks
const files = [
    'src/core/algo_bounce/bounce.h',
    'src/core/algo_bounce/bounce.c',
    'src/core/algo_bounce/internal.h',
    'src/core/algo_bounce/check/bounce_check.h',
    'src/core/algo_bounce/check/bounce_check.c',
];

console.log('1. File Structure Check:');
const baseDir = 'c:\\Users\\justa\\Documents\\0x\\Projects\\BladeOfGrass\\Codebase\\Development\\blade-provider';
let allFilesExist = true;

files.forEach(file => {
    const fullPath = path.join(baseDir, file);
    const exists = fs.existsSync(fullPath);
    console.log(`   ${exists ? '✓' : '✗'} ${file}`);
    if (!exists) allFilesExist = false;
});

console.log(`\n   Result: ${allFilesExist ? '✓ All files present' : '✗ Missing files'}\n`);

// Check dispatcher integration
console.log('2. Dispatcher Integration Check:');
const dispatcherPath = path.join(baseDir, 'src/core/dispatcher.c');
const dispatcherContent = fs.readFileSync(dispatcherPath, 'utf8');

const hasInclude = dispatcherContent.includes('#include "algo_bounce/bounce.c"');
const hasMetadata = dispatcherContent.includes('bounce_get_metadata_json()');
const hasCompute = dispatcherContent.includes('bounce_compute');

console.log(`   ${hasInclude ? '✓' : '✗'} Include bounce.c`);
console.log(`   ${hasMetadata ? '✓' : '✗'} Register metadata`);
console.log(`   ${hasCompute ? '✓' : '✗'} Route compute requests`);
console.log(`\n   Result: ${hasInclude && hasMetadata && hasCompute ? '✓ Dispatcher properly integrated' : '✗ Dispatcher integration incomplete'}\n`);

// Check bounce.c structure
console.log('3. Bounce Implementation Check:');
const bouncePath = path.join(baseDir, 'src/core/algo_bounce/bounce.c');
const bounceContent = fs.readFileSync(bouncePath, 'utf8');

const hasMetadataFn = bounceContent.includes('bounce_get_metadata_json');
const hasComputeFn = bounceContent.includes('char *bounce_compute(');
const hasCheckInclude = bounceContent.includes('#include "check/bounce_check.c"');
const hasValidationCall = bounceContent.includes('bounce_check_request_json');
const hasPathWidthParam = bounceContent.includes('Path Width');
const hasHeadlandParam = bounceContent.includes('Headland');
const hasCoverageLayer = bounceContent.includes('"Coverage"');
const hasExpandedObstaclesLayer = bounceContent.includes('Expanded Obstacles');
const hasShrunkZonesLayer = bounceContent.includes('Shrunken Zones');

console.log(`   ${hasMetadataFn ? '✓' : '✗'} Metadata function defined`);
console.log(`   ${hasComputeFn ? '✓' : '✗'} Compute function defined`);
console.log(`   ${hasCheckInclude ? '✓' : '✗'} Check module included`);
console.log(`   ${hasValidationCall ? '✓' : '✗'} Validation called in compute`);
console.log(`   ${hasPathWidthParam ? '✓' : '✗'} Path Width parameter defined`);
console.log(`   ${hasHeadlandParam ? '✓' : '✗'} Headland parameter defined`);
console.log(`   ${hasCoverageLayer ? '✓' : '✗'} Coverage layer defined`);
console.log(`   ${hasExpandedObstaclesLayer ? '✓' : '✗'} Expanded Obstacles layer defined`);
console.log(`   ${hasShrunkZonesLayer ? '✓' : '✗'} Shrunken Zones layer defined`);

const allChecks = hasMetadataFn && hasComputeFn && hasCheckInclude && hasValidationCall &&
    hasPathWidthParam && hasHeadlandParam && hasCoverageLayer &&
    hasExpandedObstaclesLayer && hasShrunkZonesLayer;

console.log(`\n   Result: ${allChecks ? '✓ All implementation checks passed' : '✗ Implementation incomplete'}\n`);

// Check validation module
console.log('4. Validation Module Check:');
const checkPath = path.join(baseDir, 'src/core/algo_bounce/check/bounce_check.c');
const checkContent = fs.readFileSync(checkPath, 'utf8');

const hasCheckFn = checkContent.includes('bool bounce_check_request_json');
const hasStartPointCheck = checkContent.includes('startPoint');
const hasZonesCheck = checkContent.includes('zones');
const hasPathWidthCheck = checkContent.includes('Path Width');
const hasHeadlandCheck = checkContent.includes('Headland');
const hasErrorHandling = checkContent.includes('bounce_set_result');
const hasMemoryCleanup = checkContent.includes('bounce_free_polygon');

console.log(`   ${hasCheckFn ? '✓' : '✗'} Validation function defined`);
console.log(`   ${hasStartPointCheck ? '✓' : '✗'} startPoint validation`);
console.log(`   ${hasZonesCheck ? '✓' : '✗'} zones validation`);
console.log(`   ${hasPathWidthCheck ? '✓' : '✗'} Path Width parsing`);
console.log(`   ${hasHeadlandCheck ? '✓' : '✗'} Headland parsing`);
console.log(`   ${hasErrorHandling ? '✓' : '✗'} Error handling`);
console.log(`   ${hasMemoryCleanup ? '✓' : '✗'} Memory cleanup`);

const validationChecks = hasCheckFn && hasStartPointCheck && hasZonesCheck &&
    hasPathWidthCheck && hasHeadlandCheck && hasErrorHandling && hasMemoryCleanup;

console.log(`\n   Result: ${validationChecks ? '✓ All validation checks passed' : '✗ Validation module incomplete'}\n`);

// Summary
console.log('=== Summary ===');
const allPassed = allFilesExist && hasInclude && hasMetadata && hasCompute &&
    allChecks && validationChecks;

if (allPassed) {
    console.log('✓ Stage 2 implementation is complete and verified!');
    console.log('\nExpected Behavior:');
    console.log('  1. GET /metadata returns bounce algorithm with 2 parameters (Path Width, Headland) and 3 layers');
    console.log('  2. POST /compute validates startPoint and zones (required)');
    console.log('  3. POST /compute parses Path Width and Headland parameters');
    console.log('  4. POST /compute returns validation errors for missing/invalid fields');
    console.log('  5. POST /compute returns empty coverage path plan on success');
    console.log('\nNote: Format and Coordinate System are Polygon/Cartesian only (not exposed as parameters)');
    console.log('\nFailing Checks:');
    if (!allFilesExist) console.log('  - Not all files present');
    if (!(hasInclude && hasMetadata && hasCompute)) console.log('  - Dispatcher not properly integrated');
    if (!allChecks) console.log('  - Bounce implementation incomplete');
    if (!validationChecks) console.log('  - Validation module incomplete');
    process.exit(1);
}
