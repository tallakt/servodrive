class ServoControl

  SERVOSOC_MAX_SERVOS = 8
  SERVOSOC_SERVO_INVALID = -1

  SERVO_MAX = 2100 # microseconds
  SERVO_MIN = 400  # microseconds
  SERVO_RANGE = SERVO_MAX - SERVO_MIN
  SERVO_ZERO = SERVO_MIN + 0.5 * SERVO_RANGE

  def initialize
    @servos = {}
  end

  # Set the speed of the individual servos. A hash is expected, with keys representing the servo number, 
  # and the values representing the speed in values between -100% and 100%
  def speed(s)
    s.each do |servo, percentage|
      raise 'Servo number should be in the range 0 to %d' % (SERVOSOC_MAX_SERVOS - 1) unless (0...SERVOSOC_MAX_SERVOS).member? servo
      raise 'percentage should be in the range -100 to 100' unless (-100..100).member? percentage

      # convert percentage to microseconds
      @servos[servo] = (percentage / 200.0 * SERVO_RANGE + SERVO_ZERO).to_i

      # servos must be written ordered by percentage value
      buffer = StringIO.new
      @servos.to_a.sort {|a, b| a.last <=> b.last }.each do |pair|
        # pair contains [servo_number, percentage]
        buffer << pair.pack('il')
      end

      no_servo = [SERVOSOC_SERVO_INVALID, 0].pack('il')
      (SERVOSOC_MAX_SERVOS - @servos.size).times { buffer << no_servo }

      File.open '/dev/servoctrl0', 'w' fo |f|
        f.write buffer.string
      end
    end
  end
end
