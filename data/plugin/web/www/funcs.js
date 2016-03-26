function long2ip(ip) {
  //  discuss at: http://phpjs.org/functions/long2ip/
  // original by: Waldo Malqui Silva
  //   example 1: long2ip( 3221234342 );
  //   returns 1: '192.0.34.166'

  if (!isFinite(ip))
    return false;

  return [ip >>> 24, ip >>> 16 & 0xFF, ip >>> 8 & 0xFF, ip & 0xFF].join('.');
}

function ip2long(IP) {
  //  discuss at: http://phpjs.org/functions/ip2long/
  // original by: Waldo Malqui Silva
  // improved by: Victor
  //  revised by: fearphage (http://http/my.opera.com/fearphage/)
  //  revised by: Theriault
  //   example 1: ip2long('192.0.34.166');
  //   returns 1: 3221234342
  //   example 2: ip2long('0.0xABCDEF');
  //   returns 2: 11259375
  //   example 3: ip2long('255.255.255.256');
  //   returns 3: false

  var i = 0;
  // PHP allows decimal, octal, and hexadecimal IP components.
  // PHP allows between 1 (e.g. 127) to 4 (e.g 127.0.0.1) components.
  IP = IP.match(
    /^([1-9]\d*|0[0-7]*|0x[\da-f]+)(?:\.([1-9]\d*|0[0-7]*|0x[\da-f]+))?(?:\.([1-9]\d*|0[0-7]*|0x[\da-f]+))?(?:\.([1-9]\d*|0[0-7]*|0x[\da-f]+))?$/i
  ); // Verify IP format.
  if (!IP) {
    return false; // Invalid format.
  }
  // Reuse IP variable for component counter.
  IP[0] = 0;
  for (i = 1; i < 5; i += 1) {
    IP[0] += !! ((IP[i] || '')
      .length);
    IP[i] = parseInt(IP[i]) || 0;
  }
  // Continue to use IP for overflow values.
  // PHP does not allow any component to overflow.
  IP.push(256, 256, 256, 256);
  // Recalculate overflow of last component supplied to make up for missing components.
  IP[4 + IP[0]] *= Math.pow(256, 4 - IP[0]);
  if (IP[1] >= IP[5] || IP[2] >= IP[6] || IP[3] >= IP[7] || IP[4] >= IP[8]) {
    return false;
  }
  return IP[1] * (IP[0] === 1 || 16777216) + IP[2] * (IP[0] <= 2 || 65536) + IP[3] * (IP[0] <= 3 || 256) + IP[4] * 1;
}

// http://stackoverflow.com/a/1199420

String.prototype.trunc = String.prototype.trunc || function(n) {
  return this.length > n ? this.substr(0, n - 1) + '&hellip;' : this;
};

// http://stackoverflow.com/a/20156012

String.prototype.hashCode = function() {
  if (Array.prototype.reduce) {
    return this.split("").reduce(function(a, b) {
      a = ((a << 5) - a) + b.charCodeAt(0);
      return a & a;
    }, 0);
  }
  var hash = 0;
  if (this.length === 0)
    return hash;
  for (var i = 0; i < this.length; i++) {
    var character  = this.charCodeAt(i);
    hash = ((hash<<5)-hash)+character;
    hash = hash & hash; // Convert to 32bit integer
  }
  return hash;
}

// http://stackoverflow.com/a/12034334

var entityMap = {
  "&": "&amp;",
  "<": "&lt;",
  ">": "&gt;",
  '"': '&quot;',
  "'": '&#39;',
  "/": '&#x2F;'
};

function escapeHtml(string) {
  return String(string).replace(/[&<>"'\/]/g, function (s) {
    return entityMap[s];
  });
}

// http://stackoverflow.com/a/8112653/1392778

function cookiesEnabled() {
  var cookieEnabled = (navigator.cookieEnabled) ? true : false;
  if (typeof navigator.cookieEnabled == "undefined" && !cookieEnabled) { 
    document.cookie = "testcookie";
    cookieEnabled = document.cookie.indexOf("testcookie") != -1;
  }
  return cookieEnabled;
}