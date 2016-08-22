var gulp        = require('gulp');
var gutil       = require('gulp-util');
var concat      = require('gulp-concat');
var inline      = require('gulp-inline');
var source      = require('vinyl-source-stream');
var babelify    = require('babelify');
var watchify    = require('watchify');
var exorcist    = require('exorcist');
var browserify  = require('browserify');
var browserSync = require('browser-sync').create();

// Input file.
watchify.args.debug = true;
var bundler_js = watchify(browserify(['./app/js/app.js'], watchify.args));

// Babel transform
bundler_js.transform(babelify.configure({
    sourceMapRelative: 'app/js',
    presets: ["es2015", "react"]
}));

// On updates recompile
bundler_js.on('update', bundle_js);

function bundle_js() {

    gutil.log('Compiling JS...');

    return bundler_js.bundle()
        .on('error', function (err) {
            gutil.log(err.message);
            browserSync.notify("Browserify Error!");
            this.emit("end");
        })
        .pipe(exorcist('app/dist/js/bundle.js.map'))
        .pipe(source('bundle.js'))
        .pipe(gulp.dest('./app/dist/js'))
        .pipe(browserSync.stream({once: true}));
}

/**
 * Gulp task alias
 */
gulp.task('bundle', function () {
    return bundle_js();
});

gulp.task('css', function () {
        return gulp.src(['app/css/main.css'])
        .pipe(concat('main.css'))
        .pipe(gulp.dest('app/dist/css'))
        .pipe(browserSync.stream({once: true}));
});

gulp.task('html', function () {
        return gulp.src(['app/index.html'])
        .pipe(concat('index.html'))
        .pipe(gulp.dest('app/dist'))
        .pipe(browserSync.stream({once: true}));
});

/**
 * First bundle, then serve from the ./app directory
 */
gulp.task('default', ['bundle','css','html'], function () {
    browserSync.init({
        server: "./app/dist"
    });
    
    gulp.watch('app/css/main.css', ['css']);
});


